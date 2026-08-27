#include "utilities/envutils.h"

#include <cstdlib>

namespace EnvUtils {

std::string Get(const std::string &name, const std::string &fallback) {
  const char *value = std::getenv(name.c_str());
  return value ? std::string(value) : fallback;
}

void Set(const std::string &name, const std::string &value) { setenv(name.c_str(), value.c_str(), 1); }

bool Has(const std::string &name) { return std::getenv(name.c_str()) != nullptr; }

}  // namespace EnvUtils
