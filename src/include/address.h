#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <variant>
namespace hamnet::ip {

// 对于 sockaddr_in.sinaddr 的封装
class address_v4 {
public:
  address_v4() noexcept;
  address_v4(const char *str);
  address_v4(std::string_view str);
  address_v4(const std::string &str);

  std::string to_string() const;
  static address_v4 make_address_v4(const char *str);
  static address_v4 make_address_v4(std::string_view str);
  static address_v4 make_address_v4(const std::string &str);

  static address_v4 any() noexcept;

  friend bool operator<(const address_v4 &lhs, const address_v4 &rhs);

  friend bool operator>(const address_v4 &lhs, const address_v4 &rhs);

  friend bool operator==(const address_v4 &lhs, const address_v4 &rhs);

  friend bool operator<=(const address_v4 &lhs, const address_v4 &rhs);

  friend bool operator>=(const address_v4 &lhs, const address_v4 &rhs);

private:
  in_addr addr_;
};

// 对于 sockaddr_in6.sin6_addr 的封装
class address_v6 {
public:
  address_v6() noexcept;
  address_v6(const char *str);
  address_v6(std::string_view str);
  address_v6(const std::string &str);

  std::string to_string() const;
  static address_v6 make_address_v6(const char *str);
  static address_v6 make_address_v6(std::string_view str);
  static address_v6 make_address_v6(const std::string &str);

  static address_v6 any() noexcept;

  friend bool operator<(const address_v6 &lhs, const address_v6 &rhs);

  friend bool operator>(const address_v6 &lhs, const address_v6 &rhs);

  friend bool operator==(const address_v6 &lhs, const address_v6 &rhs);

  friend bool operator<=(const address_v6 &lhs, const address_v6 &rhs);

  friend bool operator>=(const address_v6 &lhs, const address_v6 &rhs);

private:
  in6_addr addr_;
};

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