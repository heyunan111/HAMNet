#include <address.h>
#include <arpa/inet.h>
#include <cstring>
#include <exception.hpp>

/*
inet_pton 函数用于将字符串形式的 IP 地址转换为二进制形式。它的返回值如下：

如果函数成功，返回 1。
如果输入的地址族不支持，返回 -1，并设置 errno 为 EAFNOSUPPORT。
如果输入的 IP 地址字符串不正确，返回 0。
*/

/*
inet_ntop 函数用于将二进制形式的 IP 地址转换为字符串形式。它的返回值如下：

如果函数成功，返回一个指向结果字符串的指针。
如果发生错误，返回 NULL，并设置 errno。
*/
namespace hamnet::ip {
address_v4::address_v4() noexcept { addr_.s_addr = 0; }

address_v4::address_v4(const char *str) {
  if (0 == inet_pton(AF_INET, str, &(addr_.s_addr))) {
    throw exception::bad_ip_addr();
  }
}

address_v4::address_v4(std::string_view str) : address_v4(str.data()) {}

address_v4::address_v4(const std::string &str) : address_v4(str.c_str()) {}

std::string address_v4::to_string() const {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr_, str, INET_ADDRSTRLEN);
  return str;
}

address_v4 address_v4::make_address_v4(const char *str) {
  address_v4 addr;
  if (0 == inet_pton(AF_INET, str, &(addr.addr_))) {
    throw exception::bad_ip_addr();
  }
  return addr;
}

address_v4 address_v4::make_address_v4(std::string_view str) {
  return make_address_v4(str.data());
}

address_v4 address_v4::make_address_v4(const std::string &str) {
  return make_address_v4(str.c_str());
}

bool operator<(const address_v4 &lhs, const address_v4 &rhs) {
  return ntohl(lhs.addr_.s_addr) < ntohl(rhs.addr_.s_addr);
}

bool operator>(const address_v4 &lhs, const address_v4 &rhs) {
  return ntohl(lhs.addr_.s_addr) > ntohl(rhs.addr_.s_addr);
}

bool operator==(const address_v4 &lhs, const address_v4 &rhs) {
  return lhs.addr_.s_addr == rhs.addr_.s_addr;
}

bool operator<=(const address_v4 &lhs, const address_v4 &rhs) {
  return !(lhs > rhs);
}

bool operator>=(const address_v4 &lhs, const address_v4 &rhs) {
  return !(lhs < rhs);
}

address_v4 address_v4::any() noexcept { return address_v4(); }

address_v6::address_v6() noexcept { addr_ = in6addr_any; }

address_v6::address_v6(const char *str) {
  if (0 == inet_pton(AF_INET6, str, &addr_)) {
    throw exception::bad_ip_addr();
  }
}

address_v6::address_v6(std::string_view str) : address_v6(str.data()) {}

address_v6::address_v6(const std::string &str) : address_v6(str.c_str()) {}

std::string address_v6::to_string() const {
  char str[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, &addr_, str, INET6_ADDRSTRLEN);
  return str;
}

address_v6 address_v6::make_address_v6(const char *str) {
  address_v6 addr;
  inet_pton(AF_INET6, str, &(addr.addr_));
  return addr;
}

address_v6 address_v6::make_address_v6(std::string_view str) {
  return make_address_v6(str.data());
}

address_v6 address_v6::make_address_v6(const std::string &str) {
  return make_address_v6(str.c_str());
}

bool operator<(const address_v6 &lhs, const address_v6 &rhs) {
  return memcmp(&lhs.addr_, &rhs.addr_, sizeof(in6_addr)) < 0;
}

bool operator>(const address_v6 &lhs, const address_v6 &rhs) {
  return rhs < lhs;
}

bool operator==(const address_v6 &lhs, const address_v6 &rhs) {
  return !(lhs < rhs) && !(rhs < lhs);
}

bool operator<=(const address_v6 &lhs, const address_v6 &rhs) {
  return !(rhs < lhs);
}

bool operator>=(const address_v6 &lhs, const address_v6 &rhs) {
  return !(lhs < rhs);
}

address_v6 address_v6::any() noexcept { return address_v6(); }

address::address(const address_v4 &addr) : addr_(addr) {}
address::address(const address_v6 &addr) : addr_(addr) {}

bool address::is_v4() const {
  return std::holds_alternative<address_v4>(addr_);
}
bool address::is_v6() const {
  return std::holds_alternative<address_v6>(addr_);
}

address_v4 address::as_v4() const { return std::get<address_v4>(addr_); }
address_v6 address::as_v6() const { return std::get<address_v6>(addr_); }

std::string address::to_string() const {
  if (is_v4()) {
    return as_v4().to_string();
  } else {
    return as_v6().to_string();
  }
}

address address::make_address(const char *str) {
  if (is_ipv4(str)) {
    return address(address_v4(str));
  } else if (is_ipv6(str)) {
    return address(address_v6(str));
  } else {
    throw exception::bad_ip_addr();
  }
}

address address::make_address(std::string_view str) {
  return make_address(str.data());
}

address address::make_address(const std::string &str) {
  return make_address(str.c_str());
}

bool address::is_ipv4(const char *ip) {
  sockaddr_in sa;
  return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

bool address::is_ipv6(const char *ip) {
  sockaddr_in6 sa;
  return inet_pton(AF_INET6, ip, &(sa.sin6_addr)) != 0;
}

bool address::operator<(const address &other) const {
  if (is_v4() && other.is_v4()) {
    return as_v4() < other.as_v4();
  } else if (is_v6() && other.is_v6()) {
    return as_v6() < other.as_v6();
  } else {
    // 如果一个是 IPv4，另一个是 IPv6，我们可以认为 IPv4 < IPv6
    return is_v4();
  }
}

bool address::operator>(const address &other) const { return other < *this; }

bool address::operator==(const address &other) const {
  if (is_v4() && other.is_v4()) {
    return as_v4() == other.as_v4();
  } else if (is_v6() && other.is_v6()) {
    return as_v6() == other.as_v6();
  } else {
    return false;
  }
}

bool address::operator<=(const address &other) const {
  return !(other < *this);
}

bool address::operator>=(const address &other) const {
  return !(*this < other);
}
} // namespace hamnet::ip