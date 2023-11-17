#include <address.h>
#include <arpa/inet.h>
#include <stdexcept>

namespace hamnet::ip {

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
    throw std::invalid_argument("Invalid IP address");
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