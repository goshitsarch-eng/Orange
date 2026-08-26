#include "utilities/macaddrutils.h"

#include <fstream>

std::string MacAddrUtils::Primary() {
  std::ifstream in("/sys/class/net/eth0/address");
  std::string value;
  if (in && std::getline(in, value)) {
    return value;
  }
  return {};
}
