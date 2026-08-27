#ifndef STRAWBERRY_THREADUTILS_H
#define STRAWBERRY_THREADUTILS_H

#include <string>

namespace ThreadUtils {
std::string CurrentName();
void SetName(const std::string &name);
}  // namespace ThreadUtils

#endif
