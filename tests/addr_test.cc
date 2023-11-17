#include "address.h"
#include <gtest/gtest.h>

TEST(AddressTest, ComparisonOperators) {
  hamnet::ip::address addr1(hamnet::ip::address_v4("192.168.1.1"));
  hamnet::ip::address addr2(hamnet::ip::address_v4("192.168.1.2"));
  hamnet::ip::address addr3(hamnet::ip::address_v6("2001:db8::1"));

  // Test operator<
  EXPECT_TRUE(addr1 < addr2);
  EXPECT_FALSE(addr2 < addr1);
  EXPECT_TRUE(addr1 < addr3);
  EXPECT_FALSE(addr3 < addr1);

  // Test operator>
  EXPECT_FALSE(addr1 > addr2);
  EXPECT_TRUE(addr2 > addr1);
  EXPECT_FALSE(addr1 > addr3);
  EXPECT_TRUE(addr3 > addr1);

  // Test operator==
  EXPECT_TRUE(addr1 == addr1);
  EXPECT_FALSE(addr1 == addr2);
  EXPECT_FALSE(addr1 == addr3);

  // Test operator<=
  EXPECT_TRUE(addr1 <= addr2);
  EXPECT_FALSE(addr2 <= addr1);
  EXPECT_TRUE(addr1 <= addr3);
  EXPECT_FALSE(addr3 <= addr1);

  // Test operator>=
  EXPECT_FALSE(addr1 >= addr2);
  EXPECT_TRUE(addr2 >= addr1);
  EXPECT_FALSE(addr1 >= addr3);
  EXPECT_TRUE(addr3 >= addr1);
}

TEST(AddressTest, MakeAddress) {
  // Test make_address with IPv4 string
  hamnet::ip::address addr1 = hamnet::ip::address::make_address("192.168.1.1");
  EXPECT_TRUE(addr1.is_v4());
  EXPECT_EQ(addr1.as_v4().to_string(), "192.168.1.1");

  // Test make_address with IPv6 string
  hamnet::ip::address addr2 = hamnet::ip::address::make_address("2001:db8::1");
  EXPECT_TRUE(addr2.is_v6());
  EXPECT_EQ(addr2.as_v6().to_string(), "2001:db8::1");

  // Test make_address with invalid string
  EXPECT_THROW(hamnet::ip::address::make_address("invalid"),
               std::invalid_argument);
}

/*
hyn@laptop:~/HAMNet/bin$ ./addr_test
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 2 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 2 tests from AddressTest
[ RUN      ] AddressTest.ComparisonOperators
[       OK ] AddressTest.ComparisonOperators (0 ms)
[ RUN      ] AddressTest.MakeAddress
[       OK ] AddressTest.MakeAddress (0 ms)
[----------] 2 tests from AddressTest (0 ms total)

[----------] Global test environment tear-down
[==========] 2 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 2 tests.
*/