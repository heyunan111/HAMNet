## 1. 引言

长期以来，网络领域一直采用事件驱动和异步编程设计来开发高效、可扩展、面向网络的软件。基于proactor的事件模型，包括带有延续的异步操作，提供了一个良好的抽象和组合的概念模型。异步操作可以链接在一起，每个延续启动下一个操作。这些组合的操作可以被抽象在一个单一、更高级别的异步操作的背后，该操作具有自己的延续。

然而，随着异步组合的增加，纯回调式的方法可能会增加表面上的代码复杂性，降低可读性。程序员经常寻找替代的组合机制，如状态机、纤程，以及从C++20开始支持的基于语言的协程，以提高代码清晰度同时保留异步实现的优势。组合并非一刀切的方法。

本文提供了Asio库核心异步模型的高级概述。该模型将异步操作视为异步组合的基本构建块，但以一种将它们与组合机制解耦的方式实现。Asio中的异步操作支持回调、期望（包括急切和惰性的期望）、纤程、协程，甚至尚未想象的其他方法。这种灵活性使应用程序员能够根据适当的权衡选择一种方法。

*相同的异步操作，使用lambda表达式：*

```cpp
socket.async_read_some(buffer,
 [](error_code e, size_t)
 {
     // ...
 }
 );
```

*在协程中：*

```cpp
awaitable<void> foo()
{
 size_t n =
 co_await socket.async_read_some(
 buffer, use_awaitable
 );
 // ...
}
```

*使用期望：*

```cpp
future<size_t> f =
 socket.async_read_some(
 buffer, use_future
 );
// ...
size_t n = f.get();
```

*或者在纤程中：*

```cpp
void foo()
{
 size_t n = socket.async_read_some(
 buffer, fibers::yield
 );
 // ...
}
```

## 2 动机

### 2.1 同步操作的启发

有时，最简单的网络程序采用每连接一个线程的方法。以一个基本的回显服务器为例，纯粹采用同步操作编写：

```cpp
void echo(tcp::socket s)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = s.read_some(buffer(data));
      write(s, buffer(data, n));
    }
  }
  catch (const std::exception& e)
  {
  }
}

void listen(tcp::acceptor a)
{
  for (;;)
  {
    std::thread(echo, a.accept()).detach();
  }
}

int main()
{
  asio::io_context ctx;
  listen({ctx, {tcp::v4(), 55555}});
}
```

这个程序的结构和流程是清晰的，因为同步操作只是函数。这本质上赋予了它们几个有益的语法和语义特性，包括：

- 组合可以使用语言来管理控制流（例如 for、if、while 等）。
- 组合可以被重构为使用在同一线程上运行的子函数（即简单调用）而不改变功能。
- 如果同步操作需要临时资源（如内存、文件描述符或线程），这些资源在函数返回之前被释放。
- 当同步操作是通用的（即模板）时，返回类型可以从函数及其参数明确定义。
- 要传递给同步操作的参数的生命周期是清晰的，包括安全传递临时对象。

然而，采用每连接一个线程的方法存在一些问题，限制了它的通用适用性。

### 2.2 线程的有限可伸缩性

线程-每-连接设计，顾名思义，采用单独的线程来处理每个连接。对于处理数千，甚至数百万个并发连接的服务器而言，这在程序内部代表了显著的资源使用，尽管在近年来，64位操作系统的广泛普及已经缓解了这个问题。

然而，对于对性能敏感的使用情况而言，线程间切换的成本可能是一个更为重要的考虑因素。在通用操作系统线程之间进行上下文切换的成本以千次计。当可运行线程超过执行资源（如CPU）时，排队就会发生，而最后一个排队的任务会受到许多上下文切换成本的延迟：

![.](img\1.png)
```
最后一个线程由于前面的五个线程发生的上下文切换而延迟执行。在多线程环境中，当有多个线程竞争执行资源（比如CPU时钟周期）时，线程切换可能会引入一定的延迟。在这种情况下，最后一个被排队的任务受到了前面五个任务发生的上下文切换的影响，导致其执行被延迟。
```

即使整体上看起来网络服务器负载较轻，事件的时间相关性仍然可能导致排队。例如，在金融市场中，所有参与者都在处理和响应相同的市场数据流，因此很可能有不止一个参与者会对相同的刺激做出响应，向服务器发送交易。这种排队增加了参与者经历的平均延迟和抖动。

作为比较，专为事件处理设计的调度程序可以在几十到几百个周期内以一到两个数量级更快地在任务之间进行“上下文切换”。排队仍可能发生，但与处理队列相关的开销大大降低。

最后，我们还必须指出，我们的线程-每-连接回显服务器非常简单，因为每个线程一旦启动就能够独立运行。在实际应用场景中，服务器程序可能需要访问共享数据以响应客户端，从而产生同步成本、在CPU之间移动数据和增加代码复杂性。

### 2.3 半双工与全双工协议

对于像上面展示的回显服务器这样的简单协议，线程-每-连接方法可能是合适的，因为这是一个半双工协议。服务器要么发送，要么接收，但不同时进行两者。
然而，许多实际应用协议是全双工的，这意味着数据可以在任何时候在任何方向上传输。以FIX协议适用的一些消息交换场景为例：

![](img\2.png)
![](img\3.png)

正如你所看到的，这些协议要求对来自许多不同来源的事件进行响应。这具有一些影响：

- 并发执行的协议逻辑的不同部分可能需要访问共享状态。
- 复杂的事件处理可能不容易以线性形式表示（例如，通过每连接一个线程的设计，甚至在使用协程时也是如此）。

因此，我们经常发现这些协议的作者利用其他组合机制，比如状态机，作为一种管理复杂性并确保正确性的方式。

### 2.4 性能方面渴望的执行

某些网络应用程序要求将单个消息传递给许多消费者。一个例子是实时向所有参与者传递市场数据。在以异步方式传递此信息时，一种常见的方法是将消息封装在引用计数指针中（例如 shared_ptr），该指针保持内存有效，直到消息传递给所有参与者。

然而，为了提高效率，每个传输操作都会尝试进行一次猜测性的发送。在统计意义上，快速路径几乎总是发生（由于确保所有硬件和软件都根据预期负载正确调整的努力）。在这种情况下，猜测性的发送成功，数据立即传输。如果发生这种情况，就不再需要维护有效的共享指针。这避免了将引用计数上下调整的开销。

相比之下，原子引用计数的成本在几十个周期内，而与传输系统调用本身相比，后者在几百个周期内。避免这种额外的成本在实践中可以带来5-10%的效率提升。

惰性执行模型无法避免这种成本，因为它在首次调用操作时必须复制共享的数据。

### 2.5 设计哲学

上述问题引发了异步模型的以下设计哲学：

- **在支持组合机制方面具有灵活性**，因为适当的选择取决于具体的使用情况。
- **致力于支持尽可能多的同步操作的语义和语法属性**，因为它们能够实现更简单的组合和抽象。
- **应用代码应在很大程度上屏蔽线程和同步的复杂性**，这是因为处理来自不同来源的事件的复杂性。

## 3 模型

### 3.1 异步操作

![](img\4.png)

```chinese
在计算机术语中：
Started by:
   - 意思："Started by" 表示由某个特定实体或事件启动或发起的。这可能涉及到程序、进程、任务等的启动来源。例如，一个任务可能是由用户启动的，也可能是由另一个程序或系统事件触发的。
Calls on completion:
   - 意思："Calls on completion" 表示在完成某个操作或任务后调用的动作。这通常指的是在任务、函数或操作执行完毕后会触发的其他代码或过程。例如，在一个函数执行完成后，可能会调用其他函数或执行特定的操作。这通常与异步编程、回调函数等相关。
```

**异步操作**是Asio异步模型中组合的基本单位。异步操作代表在后台启动和执行的工作，而启动工作的用户代码可以继续执行其他任务。

从概念上讲，异步操作的生命周期可以通过以下事件和阶段来描述：

![](img\5.png)
```chinese
事件：异步操作是通过调用启动函数开始的。

事件：如果有的话，外部可观察的副作用已经完全确立。完成处理程序已排队等待执行。

事件：完成处理程序被调用，传递操作的结果。
```
**启动函数**是用户可能调用的函数，用于启动异步操作。

**完成处理器**是一个由用户提供的、只能移动的函数对象，最多会被调用一次，用异步操作的结果进行调用。完成处理器的调用告诉用户某件已经发生的事情：操作已完成，并且操作的副作用已经建立。

启动函数和完成处理器被整合到用户代码中，如下所示：

![](img\6.png)

同步操作作为单个函数具有几个固有的语义属性。异步操作从它们的同步对应物中采用了一些这些语义属性，以促进灵活和高效的组合。

| 同步操作的属性                                                                       | 异步操作的等效属性                                                                         |
| ------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------ |
| 当同步操作是通用的（即模板）时，返回类型从函数及其参数明确定义。                     | 当异步操作是通用的时，完成处理器的参数类型和顺序从启动函数及其参数明确定义。               |
| 如果同步操作需要临时资源（如内存、文件描述符或线程），这些资源在函数返回之前被释放。 | 如果异步操作需要临时资源（如内存、文件描述符或线程），这些资源在调用完成处理器之前被释放。 |

后者是异步操作的一个重要属性，因为它允许完成处理器启动进一步的异步操作，而无需重叠资源使用。考虑在链中重复执行相同操作的微不足道的（但相对常见）情况：

![](img\7.png)

通过确保在完成处理程序运行之前释放资源，我们避免了链式操作的峰值资源使用量翻倍。

### 3.2 异步代理

![dd](img\8.png)
```chinese
1. Composes:
   - 意思：将若干部分组合成一个整体。在编程中，这可能指的是通过组合多个模块、类、函数或对象来创建一个更大的系统或功能。

2. Uses:
   - 意思： 在编程中，表示一个实体（如类、模块、函数）使用另一个实体提供的功能或服务。通常表达一种依赖关系。

3. Associated with
   - 意思:与某事物相关。在编程中，可以表示两个实体之间有某种关联，但具体关联的性质可能需要根据上下文来确定。

4. Is-a:
   - 意思： 表示继承关系，通常用于面向对象编程。例如，如果类A是类B的"Is-a"关系，意味着A是B的子类，继承了B的特性。

5. Has:
   - 意思： 表示拥有某种属性、特性或关联关系。在编程中，这可以表示一个实体包含或拥有另一个实体作为其一部分。
```
一个异步代理是异步操作的顺序组合。每个异步操作都被认为是作为异步代理的一部分运行，即使该代理只包含单个操作。异步代理是一个可以与其他代理同时执行工作的实体。异步代理类似于线程与同步操作的关系。

然而，异步代理是一个纯粹的概念构造，它允许我们推理有关程序中异步操作的上下文和组合。“异步代理”这个名称在库中并没有出现，而在一个代理中组合异步操作使用了哪种具体的机制并不重要。

我们可以将异步代理可视化如下：

![dd](img\9.png)

### 3.3 关联特性和关联者

异步代理具有关联特性，指定了当将异步操作作为该代理的一部分组合时，它们应该如何行为，例如：

- **分配器（Allocator）**：确定代理的异步操作如何获取内存资源。
- **取消槽（Cancellation Slot）**：确定代理的异步操作如何支持取消。
- **一个执行器**，用于确定代理的完成处理程序将如何排队和运行。  
- 
当在异步代理内运行异步操作时，其实现可以查询这些关联特性，并使用它们来满足所表示的要求或偏好。异步操作通过将关联特性应用于完成处理程序来执行这些查询。每个特性都有一个相应的关联特性。  

关联特性可以针对具体的完成处理程序类型进行专门化，以便：  
- 接受异步操作提供的默认特性，并将其原样返回
- 返回与特性无关的实现，或
- 调整提供的默认值，以引入完成处理程序所需的附加行为。
关联器的规范
给定一个名为 associated_R 的关联特性，具有：
- 类型为 S 的源值 s，在本例中为完成处理程序及其类型，
- 一组类型要求（或概念）R，定义关联特性的语法和语义要求，
- 类型为 C 的候选值 c，满足类型要求 R，表示由异步操作提供的关联特性的默认实现
异步操作使用关联特性来计算：
- 类型 associated_R<S, C>::type，和
- 满足 R 中定义的要求的值 associated_R<S, C>::get(s, c)
为方便起见，这些也可以通过类型别名 associated_R_t<S, C> 和自由函数 get_associated_R(s, c) 访问，分别。
该特性的主模板被指定为：
- 如果 S::R_type 是良型的，则定义一个嵌套的类型别名 type 作为 S::R_type，并定义一个返回 s.get_R() 的静态成员函数 get
- 否则，如果 associator<associated_R, S, C>::type 是良型的并表示一种类型，则继承自 associator<associated_R, S, C>
- 否则，定义一个嵌套的类型别名 type 作为 C，并定义一个返回 c 的静态成员函数 get。

### 3.4 子代理

代理内的异步操作本身可能是基于子代理实现的。就父代理而言，它正在等待单个异步操作的完成。组成子代理的异步操作按顺序运行，当最终完成处理程序运行时，父代理会恢复运行。

![a](img\10.png)

与单个异步操作类似，构建在子代理上的异步操作在调用完成处理程序之前必须释放其临时资源。我们也可以将这些子代理视为在完成处理程序被调用之前结束其生命周期的资源。

当一个异步操作创建一个子代理时，它可以将父代理的关联特性传播到子代理。然后，这些关联特性可以递归地通过进一步的异步操作和子代理传播。这种异步操作的层叠复制了同步操作的另一属性。

| 同步操作的属性                                                                           | 异步操作的等效属性                                                               |
| ---------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------- |
| 同步操作的组合可以进行重构，以使用在同一线程上运行的子函数（即简单调用），而不改变功能。 | 异步代理可以进行重构，以使用共享父代理关联特性的异步操作和子代理，而不改变功能。 |

最后，一些异步操作可以根据多个并发运行的子代理实现。在这种情况下，异步操作可以选择性地传播父代理的关联特性。

### 3.5执行器

每个异步代理都有一个关联的执行器。代理的执行器决定了代理的完成处理程序如何被排队和最终运行。

执行器的示例用途包括：

- 协调一组在共享数据结构上操作的异步代理，确保代理的完成处理程序永远不会并发运行。
- 确保代理在指定的执行资源（例如，CPU）上运行，该资源靠近数据或事件源（例如，网络接口卡）。
- 表示一组相关的代理，从而使动态线程池能够做出更智能的调度决策（例如，将代理作为一个单元在执行资源之间移动）。
- 将所有完成处理程序排队以在 GUI 应用程序线程上运行，以便它们可以安全地更新用户界面元素。
- 将异步操作的默认执行器原样返回，以便尽可能靠近触发操作完成的事件运行完成处理程序。
- 调整异步操作的默认执行器，以在每个完成处理程序之前和之后运行代码，例如日志记录、用户授权或异常处理。
- 为异步代理及其完成处理程序指定优先级。
异步代理内的异步操作使用代理的关联执行器来实现以下目的：
- 在操作未完成时跟踪异步操作所代表的工作的存在。
- 将完成处理程序排队以在操作完成时执行。
- 确保完成处理程序不会重入运行，以防这样做可能导致意外的递归和堆栈溢出。
因此，异步代理的关联执行器代表了代理应该如何、在何处以及何时运行的策略，作为构成代理的代码的横切关注点。

### 3.6 分配器

每个异步代理都有一个关联的分配器。代理的分配器是代理的异步操作用于获取每个操作稳定内存资源（POSMs）的接口。这个名称反映了内存是每个操作的，因为内存仅在该操作的生命周期内保留，而且是稳定的，因为内存保证在整个操作期间在那个位置可用。

异步操作可以以多种不同的方式利用POSMs：
- 该操作不需要任何POSMs。例如，该操作包装一个执行自己的内存管理的现有API，或将长寿命周期状态复制到现有内存中，如循环缓冲区。
- 该操作使用一个固定大小的POSM，只要该操作未完成。例如，该操作将某些状态存储在链表中。
- 该操作使用一个运行时大小的POSM。例如，该操作存储用户提供的缓冲区的副本，或者一个iovec结构的运行时大小数组。
- 该操作同时使用多个POSMs。例如，链表的固定大小POSM加上缓冲区的运行时大小POSM。
- 该操作串行使用多个POSMs，其大小可能不同。

关联的分配器允许用户将POSM优化视为异步操作组合的横切关注点。此外，将分配器用作获取POSMs的接口为异步操作的实现者和用户提供了相当大的灵活性：
- 用户可以忽略分配器，接受应用程序采用的任何默认策略。
- 实现者可以忽略分配器，特别是如果操作不被认为是性能敏感的。
- 用户可以为相关的异步操作共享POSMs，以获得更好的引用局部性。
- 对于涉及不同大小的串行POSMs的组合，内存使用量只需与当前存在的POSM一样大。例如，考虑一个包含使用大型POSMs（建立连接和握手）的短暂操作，然后是使用小型POSMs（与对等体传输数据）的长期操作。

如前所述，必须在调用完成处理程序之前释放所有资源。这使得内存可以在代理内的后续异步操作中进行回收。这允许具有长寿命周期异步代理的应用程序没有热路径内存分配，即使用户代码对关联的分配器不知情。

### 3.7 取消操作

在Asio中，许多对象（例如套接字和定时器）通过它们的`close`或`cancel`成员函数支持对整个对象范围内的未完成异步操作进行取消。然而，某些异步操作还支持单独的、有针对性的取消。这种每个操作的取消是通过指定每个异步代理都有一个关联的取消插槽来实现的。

为了支持取消，异步操作将一个取消处理程序安装到代理的插槽中。取消处理程序是一个函数对象，当用户向插槽发出取消信号时将被调用。由于取消插槽与单个代理相关联，插槽最多同时保存一个处理程序，安装新的处理程序将覆盖先前安装的任何处理程序。因此，在代理内的后续异步操作中重复使用相同的插槽。

当异步操作包含多个子代理时，取消操作特别有用。例如，一个子代理可能已完成，而另一个则被取消，因为它的副作用不再需要。

### 3.8 完成令牌
![d](img\11.png)

Asio的异步模型的一个关键目标是支持多种组合机制。这是通过使用完成令牌来实现的，用户将完成令牌传递给异步操作的启动函数，以定制库的API表面。按照惯例，完成令牌是异步操作启动函数的最后一个参数。

例如，如果用户将一个 lambda 表达式（或其他函数对象）作为完成令牌传递，异步操作将表现为前面描述的方式：操作开始，当操作完成时，结果将传递给 lambda 表达式。
```cpp
socket.async_read_some(buffer,
    [](std::error_code error, std::size_t bytes_transferred) {
        if (!error) {

        } else {

        }
    }
);
```

当用户传递 `use_future` 完成令牌时，该操作的行为就好像是基于 promise 和 future 对的实现一样。启动函数不仅启动操作，还返回一个 `std::future`，可以用于异步等待结果。

以下是使用 `use_future` 完成令牌的示例：
```cpp
std::future<std::size_t> f = socket.async_read_some(buffer, use_future);

// ... Perform other tasks concurrently ...

size_t n = f.get(); // Await the result
// Use 'n' to handle the completed asynchronous read operation
```

类似地，当用户传递 `use_awaitable` 完成令牌时，启动函数的行为就好像是基于可等待协程的实现一样。然而，在这种情况下，启动函数不会直接启动异步操作。它只返回可等待对象，在进行 `co_await` 操作时启动操作。

```cpp
awaitable<void> foo()
{
    size_t n = co_await socket.async_read_some(buffer, use_awaitable);
    // ...
}
```

最后，`fibers::yield` 完成令牌导致启动函数表现得像一个了解纤程的同步操作。它不仅启动异步操作，而且会阻塞纤程，直到操作完成。从纤程的角度看，它是一个同步操作。

```cpp
void foo()
{
    size_t n = socket.async_read_some(buffer, fibers::yield);
    // ...
}
```

所有这些用法都由`async_read_some`启动函数的单一实现支持。为了实现这一点，异步操作首先必须指定一个完成签名（或者可能是多个签名），该签名描述了将传递给其完成处理程序的参数。然后，操作的启动函数将完成签名、完成令牌和其内部实现传递给 `async_result` 特性。`async_result` 特性是一个定制点，将这些组合起来，首先生成一个具体的完成处理程序，然后启动操作。

![](img\12..png)

在实践中，让我们使用一个分离的线程将同步操作转换为异步操作：

```cpp
// 异步操作结果的 trait，将异步操作的完成处理程序与底层实现连接起来
template <class CompletionToken>
auto async_read_some(tcp::socket& s, const mutable_buffer& b, CompletionToken&& token)
{
    // 初始化函数，用于启动异步操作
    auto init = [](
        auto completion_handler,
        tcp::socket* s,
        const mutable_buffer& b)
    {
        // 在新线程中执行异步操作
        std::thread(
            [](
                auto completion_handler,
                tcp::socket* s,
                const mutable_buffer& b
            )
            {
                // 异步操作的实际实现，这里使用 read_some 作为例子
                error_code ec;
                size_t n = s->read_some(b, ec);

                // 调用完成处理程序，将结果传递给用户
                std::move(completion_handler)(ec, n);
            },
            std::move(completion_handler), // 移动完成处理程序
            s,
            b
        ).detach(); // 分离线程，使其独立运行
    };

    // 使用 async_result trait 来启动异步操作
    return async_result<
        decay_t<CompletionToken>,
        void(error_code, size_t) // 完成处理程序的签名
    >::initiate(
        init,
        std::forward<CompletionToken>(token), // 完成令牌
        &s, // 异步操作的其他参数
        b
    );
}
```

我们可以将完成令牌视为一种原型完成处理程序。在将函数对象（如 lambda）作为完成令牌传递的情况下，它已经满足完成处理程序的要求。`async_result` 主模板通过简单地将参数转发处理这种情况：

```cpp
template <class CompletionToken, completion_signature... Signatures>
struct async_result
{
    template <
        class Initiation,
        completion_handler_for<Signatures...> CompletionHandler,
        class... Args>
    static void initiate(
        Initiation&& initiation,
        CompletionHandler&& completion_handler,
        Args&&... args)
    {
        std::forward<Initiation>(initiation)(
            std::forward<CompletionHandler>(completion_handler),
            std::forward<Args>(args)...);
    }
};
```

这个例子仅用于说明目的，不建议在实际应用中使用。

我们可以看到，这个默认实现避免了所有参数的复制，从而确保急切启动尽可能高效。另一方面，懒惰的完成令牌（例如上面的 `use_awaitable`）可能捕获这些参数以延迟启动操作。例如，对于一个简单的延迟令牌的特化（仅仅是将操作包装起来以供以后使用），可能如下所示：
```cpp
// 默认实现避免了所有参数的复制，确保急切启动尽可能高效。
// 另一方面，懒惰的完成令牌（如上面的 use_awaitable）可能捕获这些参数以延迟启动操作。
// 例如，对于一个简单的延迟令牌的特化（仅简单地打包操作以供以后使用），可能如下所示：

template <completion_signature... Signatures>
struct async_result<deferred_t, Signatures...>
{
    template <class Initiation, class... Args>
    static auto initiate(Initiation initiation, deferred_t, Args... args)
    {
        return [
            initiation = std::move(initiation),
            arg_pack = std::make_tuple(std::move(args)...)
        ](auto&& token) mutable
        {
            return std::apply(
                [&](auto&&... args)
                {
                    return async_result<decay_t<decltype(token)>, Signatures...>::initiate(
                        std::move(initiation),
                        std::forward<decltype(token)>(token),
                        std::forward<decltype(args)>(args)...
                    );
                },
                std::move(arg_pack)
            );
        };
    }
};
```

### 3.9 摘要库元素

- `completion_signature` 概念：定义有效的完成签名形式。
- `completion_handler_for` 概念：确定给定的完成处理程序是否可以调用特定的完成签名集。
- `async_result` 特性：将完成签名和完成令牌转换为具体的完成处理程序，并启动操作。
- `async_initiate` 函数：简化 `async_result` 特性的使用的辅助函数。
- `associator` 特性：通过抽象层次传播所有关联者的特性。
- `associated_executor` 特性：定义异步代理的关联执行器。
- `associated_executor_t` 模板类型别名。
- `get_associated_executor` 函数。
- `associated_allocator` 特性：定义异步代理的关联分配器。
- `associated_allocator_t` 模板类型别名。
- `get_associated_allocator` 函数。
- `associated_cancellation_slot` 特性：定义异步代理的关联取消插槽。
- `associated_cancellation_slot_t` 模板类型别名。
- `get_associated_cancellation_slot` 函数。

### 3.10 高级抽象
本文介绍的异步模型为定义更高级的抽象提供了基础，但规范这些抽象被认为超出了本文的范围。相反，它的范围仅限于规范构成这种组合的异步操作。

然而，Asio库已经在这个核心模型的基础上构建，提供了这些额外的功能，例如：
- I/O 对象，如套接字和定时器，在该模型之上公开异步操作。
- 具体的执行器，如 io_context 执行器、thread_pool 执行器和 strand 适配器，保证完成处理程序的非并发执行。
- 完成令牌，促进不同的组合机制，如协程、纤程、期货和延迟操作。
- 对C++协程的高级支持，结合了对执行器和取消插槽的支持，以便轻松协调并发异步代理。

## 4 示例

许多构建在这个核心异步模型之上的示例可以在Asio库的分发中找到，以及在许多构建在Asio之上的应用程序和库中找到。然而，为了说明如何在这个模型之上构建高级组合，以下示例利用了与Asio一起提供的C++20协程支持。

### 开始之前

让我们首先比较最初的每连接一个线程的回显服务器和基于协程的回显服务器。

#### 每连接一个线程的回显服务器

```cpp
#include <asio.hpp>
using asio::buffer;
using asio::ip::tcp;

void echo(tcp::socket s)
{
    try
    {
        char data[1024];
        for (;;)
        {
            // 从套接字读取数据
            std::size_t n = s.read_some(buffer(data));
            
            // 将数据回写到套接字
            write(s, buffer(data, n));
        }
    }
    catch (const std::exception& e)
    {
        // 异常处理
    }
}

void listen(tcp::acceptor a)
{
    for (;;)
    {
        // 启动新线程处理连接
        std::thread(echo, a.accept()).detach();
    }
}

int main()
{
    asio::io_context ctx;
    
    // 启动监听
    listen({ctx, {tcp::v4(), 55555}});
}
```

#### 基于协程的回显服务器

```cpp
#include <asio.hpp>
using asio::awaitable;
using asio::buffer;
using asio::detached;
using asio::ip::tcp;
using asio::use_awaitable;

awaitable<void> echo(tcp::socket s)
{
    try
    {
        char data[1024];
        for (;;)
        {
            // 使用协程方式异步读取数据
            std::size_t n = co_await s.async_read_some(buffer(data), use_awaitable);
            
            // 使用协程方式异步写入数据
            co_await async_write(s, buffer(data, n), use_awaitable);
        }
    }
    catch (const std::exception& e)
    {
        // 异常处理
    }
}

awaitable<void> listen(tcp::acceptor a)
{
    for (;;)
    {
        // 使用协程方式异步接受连接，并启动新协程处理连接
        co_spawn(a.get_executor(), echo(co_await a.async_accept(use_awaitable)), detached);
    }
}

int main()
{
    asio::io_context ctx;
    
    // 使用协程方式启动监听
    co_spawn(ctx, listen({ctx, {tcp::v4(), 55555}}), detached);
    
    // 运行 IO 上下文
    ctx.run();
}
```

这展示了协程支持如何利用该模型来复制同步操作的语义和语法属性。

### TCP套接字代理示例

这是一个简单的TCP套接字代理的片段。它演示了协程如何与取消支持结合，以提供并发异步代理的优雅协同，以及有效的超时支持：

```cpp
constexpr auto use_nothrow_awaitable = as_tuple(use_awaitable);

// 异步数据传输协程
awaitable<void> transfer(tcp::socket& from, tcp::socket& to, steady_clock::time_point& deadline)
{
    std::array<char, 1024> data;
    for (;;)
    {
        // 更新截止时间
        deadline = std::max(deadline, steady_clock::now() + 5s);

        // 异步读取数据
        auto [e1, n1] = co_await from.async_read_some(buffer(data), use_nothrow_awaitable);
        if (e1)
            co_return;

        // 异步写入数据
        auto [e2, n2] = co_await async_write(to, buffer(data, n1), use_nothrow_awaitable);
        if (e2)
            co_return;
    }
}

// 监视协程，用于实现超时
awaitable<void> watchdog(steady_clock::time_point& deadline)
{
    steady_timer timer(co_await this_coro::executor);
    auto now = steady_clock::now();
    while (deadline > now)
    {
        // 设置定时器到期时间
        timer.expires_at(deadline);
        // 等待定时器异步事件
        co_await timer.async_wait(use_nothrow_awaitable);
        now = steady_clock::now();
    }
}

// 代理协程，负责连接和数据传输
awaitable<void> proxy(tcp::socket client, tcp::endpoint target)
{
    tcp::socket server(client.get_executor());
    steady_clock::time_point deadline{};

    // 异步连接到目标服务器
    auto [e] = co_await server.async_connect(target, use_nothrow_awaitable);
    if (!e)
    {
        // 使用协程同时处理数据传输和超时监视
        co_await (
            transfer(client, server, deadline) ||
            transfer(server, client, deadline) ||
            watchdog(deadline)
        );
    }
}

// 异步监听协程
awaitable<void> listen(tcp::acceptor& acceptor, tcp::endpoint target)
{
    for (;;)
    {
        // 异步接受客户端连接
        auto [e, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
        if (e)
            break;

        // 获取客户端执行器并启动代理协程
        auto ex = client.get_executor();
        co_spawn(ex, proxy(std::move(client), target), detached);
    }
}
```

### 基于协程的按名称连接示例

这是一个演示按名称连接的协程实现片段。该算法尝试并行连接多个主机，一旦成功一个，剩余的操作会自动取消。

```cpp
tcp::socket selected(std::variant<tcp::socket, tcp::socket> v)
{
    switch (v.index())
    {
    case 0:
        return std::move(std::get<0>(v));
    case 1:
        return std::move(std::get<1>(v));
    default:
        throw std::logic_error(__func__);
    }
}

// 异步连接单个端点
awaitable<tcp::socket> connect(ip::tcp::endpoint ep)
{
    auto sock = tcp::socket(co_await this_coro::executor);
    co_await sock.async_connect(ep, use_awaitable);
    co_return std::move(sock);
}

// 异步连接端点范围
awaitable<tcp::socket> connect_range(
    tcp::resolver::results_type::const_iterator first,
    tcp::resolver::results_type::const_iterator last)
{
    assert(first != last);

    auto next = std::next(first);
    if (next == last)
        co_return co_await connect(first->endpoint());
    else
        co_return selected(co_await (connect(first->endpoint()) || connect_range(next, last)));
}

// 按名称异步连接
awaitable<tcp::socket> connect_by_name(std::string host, std::string service)
{
    auto resolver = tcp::resolver(co_await this_coro::executor);
    auto results = co_await resolver.async_resolve(host, service, use_awaitable);
    co_return co_await connect_range(results.begin(), results.end());
}
```

这个片段展示了一个通过名称连接的协程实现，它并行尝试连接多个主机，一旦成功一个，就会自动取消剩余的操作。

## 5核心异步模型示例实现

以下是从Asio源代码中衍生的核心异步模型实现。为了清晰起见，它已更新为使用C++20特性，并删除了向后兼容的功能。
```cpp
// completion_signature 概念
template <class T>
struct __is_completion_signature : false_type {};
template <class R, class... Args>
struct __is_completion_signature<R(Args...)> : true_type {};
template <class T>
concept completion_signature = __is_completion_signature<T>::value;

// completion_handler_for 概念
template <class T, class R, class... Args>
struct __is_completion_handler_for : is_invocable<decay_t<T>&&, Args...> {};

template <class T, class... Signatures>
concept completion_handler_for = (completion_signature<Signatures> && ...)
    && (__is_completion_handler_for<T, Signatures>::value && ...);

// async_result trait
template <class CompletionToken, completion_signature... Signatures>
struct async_result
{
    template <
        class Initiation,
        completion_handler_for<Signatures...> CompletionHandler,
        class... Args>
    static void initiate(
        Initiation&& initiation,
        CompletionHandler&& completion_handler,
        Args&&... args)
    {
        std::forward<Initiation>(initiation)(
            std::forward<CompletionHandler>(completion_handler),
            std::forward<Args>(args)...);
    }
};

// async_initiate 辅助函数
template <
    class CompletionToken,
    completion_signature... Signatures,
    class Initiation,
    class... Args>
auto async_initiate(
    Initiation&& initiation,
    type_identity_t<CompletionToken>& token,
    Args&&... args)
    -> decltype(
        async_result<decay_t<CompletionToken>, Signatures...>::initiate(
            std::forward<Initiation>(initiation),
            std::forward<CompletionToken>(token),
            std::forward<Args>(args)...))
{
    return async_result<decay_t<CompletionToken>, Signatures...>::initiate(
        std::forward<Initiation>(initiation),
        std::forward<CompletionToken>(token),
        std::forward<Args>(args)...);
}

// completion_token_for 概念
template <class... Signatures>
struct __initiation_archetype
{
    template <completion_handler_for<Signatures...> CompletionHandler>
    void operator()(CompletionHandler&&) const {}
};

template <class T, class... Signatures>
concept completion_token_for = (completion_signature<Signatures> && ...)
    && requires(T&& t)
{
    async_initiate<T, Signatures...>(__initiation_archetype<Signatures...>{}, t);
};

// associator trait
template <template <class, class> class Associator, class S, class C>
struct associator {};

// associated_executor trait
template <class S, class C>
struct associated_executor
{
    // 获取关联的执行器
    static auto __get(const S& s, const C& c) noexcept
    {
        // 如果 S 类型有 executor_type，则使用 s.get_executor()
        if constexpr (requires { typename S::executor_type; })
            return s.get_executor();
        // 如果有 associator<associated_executor, S, C>::type，则使用 associator 的 get 函数
        else if constexpr (requires { typename associator<associated_executor, S, C>::type; })
            return associator<associated_executor, S, C>::get(s, c);
        // 否则使用默认的 c
        else
            return c;
    }

    using type = decltype(associated_executor::__get(declval<const S&>(), declval<const C&>()));
    // 获取关联的执行器
    static type get(const S& s, const C& c) noexcept
    {
        return __get(s, c);
    }
};

template <class S, class C>
using associated_executor_t = typename associated_executor<S, C>::type;

// 获取关联的执行器
template <class S, class C>
associated_executor_t<S, C> get_associated_executor(const S& s, const C& c) noexcept
{
    return associated_executor<S, C>::get(s, c);
}

// associated_allocator trait
template <class S, class C>
struct associated_allocator
{
    // 获取关联的分配器
    static auto __get(const S& s, const C& c) noexcept
    {
        // 如果 S 类型有 allocator_type，则使用 s.get_allocator()
        if constexpr (requires { typename S::allocator_type; })
            return s.get_allocator();
        // 如果有 associator<associated_allocator, S, C>::type，则使用 associator 的 get 函数
        else if constexpr (requires { typename associator<associated_allocator, S, C>::type; })
            return associator<associated_allocator, S, C>::get(s, c);
        // 否则使用默认的 c
        else
            return c;
    }

    using type = decltype(associated_allocator::__get(declval<const S&>(), declval<const C&>()));
   

 // 获取关联的分配器
    static type get(const S& s, const C& c) noexcept
    {
        return __get(s, c);
    }
};

template <class S, class C>
using associated_allocator_t = typename associated_allocator<S, C>::type;

// 获取关联的分配器
template <class S, class C>
associated_allocator_t<S, C> get_associated_allocator(const S& s, const C& c) noexcept
{
    return associated_allocator<S, C>::get(s, c);
}

// associated_cancellation_slot trait
template <class S, class C>
struct associated_cancellation_slot
{
    // 获取关联的取消槽
    static auto __get(const S& s, const C& c) noexcept
    {
        // 如果 S 类型有 cancellation_slot_type，则使用 s.get_cancellation_slot()
        if constexpr (requires { typename S::cancellation_slot_type; })
            return s.get_cancellation_slot();
        // 如果有 associator<associated_cancellation_slot, S, C>::type，则使用 associator 的 get 函数
        else if constexpr (requires { typename associator<associated_cancellation_slot, S, C>::type; })
            return associator<associated_cancellation_slot, S, C>::get(s, c);
        // 否则使用默认的 c
        else
            return c;
    }

    using type = decltype(associated_cancellation_slot::__get(declval<const S&>(), declval<const C&>()));
    // 获取关联的取消槽
    static type get(const S& s, const C& c) noexcept
    {
        return __get(s, c);
    }
};

template <class S, class C>
using associated_cancellation_slot_t = typename associated_cancellation_slot<S, C>::type;

// 获取关联的取消槽
template <class S, class C>
associated_cancellation_slot_t<S, C> get_associated_cancellation_slot(const S& s, const C& c) noexcept
{
    return associated_cancellation_slot<S, C>::get(s, c);
}
```
