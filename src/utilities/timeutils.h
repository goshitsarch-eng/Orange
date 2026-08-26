#ifndef STRAWBERRY_TIMEUTILS_H
#define STRAWBERRY_TIMEUTILS_H

#include <cstdint>
#include <string>

namespace Utilities {

std::string PrettyTime(int seconds);
std::string PrettyTimeDelta(int seconds);
std::string PrettyTimeNanosec(int64_t nanoseconds);
std::string WordyTime(uint64_t seconds);
std::string WordyTimeNanosec(uint64_t nanoseconds);

}  // namespace Utilities

#endif
