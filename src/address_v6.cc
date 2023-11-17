#include <address_v6.h>
#include <cstring>
namespace hamnet::ip {

address_v6::address_v6() noexcept { addr_ = in6addr_any; }

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

} // namespace hamnet::ip