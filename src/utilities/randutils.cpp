#include "utilities/randutils.h"

#include <glib.h>

namespace RandUtils {

std::string GetRandomString(int len, const char *alphabet) {
  std::string out;
  out.reserve(len);
  const int n = static_cast<int>(g_utf8_strlen(alphabet, -1));
  for (int i = 0; i < len; ++i) {
    out.push_back(alphabet[g_random_int_range(0, n)]);
  }
  return out;
}

std::string GetRandomStringWithChars(int len) { return GetRandomString(len, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"); }

std::string GetRandomStringWithCharsAndNumbers(int len) {
  return GetRandomString(len, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

std::string CryptographicRandomString(int len) { return GetRandomStringWithCharsAndNumbers(len); }

}  // namespace RandUtils
