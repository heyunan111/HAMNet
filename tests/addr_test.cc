#include "address.h"
#include <gtest/gtest.h>

using namespace hamnet::ip;

TEST(AddressV4Test, ConstructorAndToString) {
  // Test constructor with valid string
  address_v4 addr1("192.168.1.1");
  EXPECT_EQ(addr1.to_string(), "192.168.1.1");

  // Test constructor with std::string
  std::string ip_str = "192.168.1.2";
  address_v4 addr2(ip_str);
  EXPECT_EQ(addr2.to_string(), "192.168.1.2");

  // Test constructor with std::string_view
  std::string_view ip_str_view = "192.168.1.3";
  address_v4 addr3(ip_str_view);
  EXPECT_EQ(addr3.to_string(), "192.168.1.3");
}

TEST(AddressV4Test, MakeAddressV4) {
  // Test make_address_v4 with valid string
  address_v4 addr1 = address_v4::make_address_v4("192.168.1.1");
  EXPECT_EQ(addr1.to_string(), "192.168.1.1");

  // Test make_address_v4 with std::string
  std::string ip_str = "192.168.1.2";
  address_v4 addr2 = address_v4::make_address_v4(ip_str);
  EXPECT_EQ(addr2.to_string(), "192.168.1.2");

  // Test make_address_v4 with std::string_view
  std::string_view ip_str_view = "192.168.1.3";
  address_v4 addr3 = address_v4::make_address_v4(ip_str_view);
  EXPECT_EQ(addr3.to_string(), "192.168.1.3");
}

TEST(AddressV4Test, ComparisonOperators) {
  address_v4 addr1("192.168.1.1");
  address_v4 addr2("192.168.1.2");

  // Test operator<
  EXPECT_TRUE(addr1 < addr2);
  EXPECT_FALSE(addr2 < addr1);

  // Test operator>
  EXPECT_FALSE(addr1 > addr2);
  EXPECT_TRUE(addr2 > addr1);

  // Test operator==
  EXPECT_TRUE(addr1 == addr1);
  EXPECT_FALSE(addr1 == addr2);

  // Test operator<=
  EXPECT_TRUE(addr1 <= addr2);
  EXPECT_FALSE(addr2 <= addr1);

  // Test operator>=
  EXPECT_FALSE(addr1 >= addr2);
  EXPECT_TRUE(addr2 >= addr1);
}
