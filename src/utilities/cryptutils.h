#ifndef STRAWBERRY_CRYPTUTILS_H
#define STRAWBERRY_CRYPTUTILS_H

#include <string>

namespace CryptUtils {

std::string HmacSha1(const std::string &key, const std::string &data);
std::string HmacSha256(const std::string &key, const std::string &data);
std::string HexEncode(const std::string &data);

}  // namespace CryptUtils

#endif
