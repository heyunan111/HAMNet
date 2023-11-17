#include <address_v4.h>

namespace hamnet::ip {

address_v4::address_v4() noexcept { addr_.s_addr = 0; }

std::string address_v4::to_string() const {
  char str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr_, str, INET_ADDRSTRLEN);
  return str;
}

address_v4 address_v4::make_address_v4(const char *str) {
  address_v4 addr;
  inet_pton(AF_INET, str, &(addr.addr_));
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

} // namespace hamnet::ip