#ifndef STRAWBERRY_RANDUTILS_H
#define STRAWBERRY_RANDUTILS_H

#include <string>

namespace RandUtils {

std::string GetRandomStringWithChars(int len);
std::string GetRandomStringWithCharsAndNumbers(int len);
std::string CryptographicRandomString(int len);

}  // namespace RandUtils

#endif
