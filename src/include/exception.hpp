#include <exception>

namespace hamnet::exception {

class bad_ip_addr : public std::exception {
public:
  bad_ip_addr() {}

  const char *what() const noexcept override { return "Invalid IP address"; }
};

} // namespace hamnet::exception