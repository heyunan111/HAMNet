#pragma once

#include <address_v4.h>
#include <address_v6.h>
#include <string_view>
#include <variant>

namespace hamnet::ip {

class address {
public:
  address(const address_v4 &addr);
  address(const address_v6 &addr);

  bool is_v4() const;
  bool is_v6() const;

  address_v4 as_v4() const;
  address_v6 as_v6() const;

  std::string to_string() const;

  static address make_address(const char *str);
  static address make_address(std::string_view str);
  static address make_address(const std::string &str);

  bool operator<(const address &other) const;
  bool operator>(const address &other) const;
  bool operator==(const address &other) const;
  bool operator<=(const address &other) const;
  bool operator>=(const address &other) const;

private:
  static bool is_ipv4(const char *ip);
  static bool is_ipv6(const char *ip);

  std::variant<address_v4, address_v6> addr_;
};

} // namespace hamnet::ip