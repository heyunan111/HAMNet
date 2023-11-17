#include <address_v4.h>
#include <address_v6.h>
#include <gtest/gtest.h>

using namespace hamnet::ip;

// address_v4 测试
TEST(AddressTest, V4DefaultConstructor) {
  address_v4 addr;
  EXPECT_EQ(addr.to_string(), "0.0.0.0");
}

TEST(AddressTest, V4MakeAddress) {
  address_v4 addr = address_v4::make_address_v4("127.0.0.1");
  EXPECT_EQ(addr.to_string(), "127.0.0.1");
}

TEST(AddressTest, V4ComparisonOperators) {
  address_v4 addr1 = address_v4::make_address_v4("192.168.1.1");
  address_v4 addr2 = address_v4::make_address_v4("192.168.1.2");
  EXPECT_TRUE(addr1 < addr2);
  EXPECT_FALSE(addr1 > addr2);
  EXPECT_FALSE(addr1 == addr2);
  EXPECT_TRUE(addr1 <= addr2);
  EXPECT_FALSE(addr1 >= addr2);
}

// address_v6 测试
TEST(AddressTest, V6DefaultConstructor) {
  address_v6 addr;
  EXPECT_EQ(addr.to_string(), "::");
}

TEST(AddressTest, V6MakeAddress) {
  address_v6 addr = address_v6::make_address_v6("::1");
  EXPECT_EQ(addr.to_string(), "::1");
}

TEST(AddressTest, V6ComparisonOperators) {
  address_v6 addr1 = address_v6::make_address_v6("2001:db8::1");
  address_v6 addr2 = address_v6::make_address_v6("2001:db8::2");
  EXPECT_TRUE(addr1 < addr2);
  EXPECT_FALSE(addr1 > addr2);
  EXPECT_FALSE(addr1 == addr2);
  EXPECT_TRUE(addr1 <= addr2);
  EXPECT_FALSE(addr1 >= addr2);
}

/*
hyn@laptop:~/HAMNet/bin$ /home/hyn/HAMNet/bin/addr_v4_v6_test
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 6 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 6 tests from AddressTest
[ RUN      ] AddressTest.V4DefaultConstructor
[       OK ] AddressTest.V4DefaultConstructor (0 ms)
[ RUN      ] AddressTest.V4MakeAddress
[       OK ] AddressTest.V4MakeAddress (0 ms)
[ RUN      ] AddressTest.V4ComparisonOperators
[       OK ] AddressTest.V4ComparisonOperators (0 ms)
[ RUN      ] AddressTest.V6DefaultConstructor
[       OK ] AddressTest.V6DefaultConstructor (0 ms)
[ RUN      ] AddressTest.V6MakeAddress
[       OK ] AddressTest.V6MakeAddress (0 ms)
[ RUN      ] AddressTest.V6ComparisonOperators
[       OK ] AddressTest.V6ComparisonOperators (0 ms)
[----------] 6 tests from AddressTest (0 ms total)

[----------] Global test environment tear-down
[==========] 6 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 6 tests.
*/