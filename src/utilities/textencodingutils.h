#ifndef STRAWBERRY_TEXTENCODINGUTILS_H
#define STRAWBERRY_TEXTENCODINGUTILS_H

#include <string>

namespace TextEncodingUtils {
std::string ToUtf8(const std::string &value, const std::string &from_encoding = {});
bool IsUtf8(const std::string &value);
}  // namespace TextEncodingUtils

#endif
