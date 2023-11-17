#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <string_view>

namespace hamnet::ip {

// 对于 sockaddr_in.sinaddr 的封装
class address_v4 {
public:
  address_v4() noexcept;
  address_v4(const char *str) noexcept;
  address_v4(std::string_view str) noexcept;
  address_v4(const std::string &str) noexcept;

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
} // namespace hamnet::ip
