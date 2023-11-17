#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <string_view>

namespace hamnet::ip {

// 对于 sockaddr_in6.sin6_addr 的封装
class address_v6 {
public:
  address_v6() noexcept;
  address_v6(const char *str) noexcept;
  address_v6(std::string_view str) noexcept;
  address_v6(const std::string &str) noexcept;

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

} // namespace hamnet::ip
