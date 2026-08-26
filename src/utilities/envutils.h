#ifndef STRAWBERRY_ENVUTILS_H
#define STRAWBERRY_ENVUTILS_H

#include <string>

namespace EnvUtils {

std::string Get(const std::string &name, const std::string &fallback = {});
void Set(const std::string &name, const std::string &value);
bool Has(const std::string &name);

}  // namespace EnvUtils

#endif
