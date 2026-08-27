#include "utilities/textencodingutils.h"

#include <glib.h>

bool TextEncodingUtils::IsUtf8(const std::string &value) { return g_utf8_validate(value.c_str(), static_cast<gssize>(value.size()), nullptr); }

std::string TextEncodingUtils::ToUtf8(const std::string &value, const std::string &from_encoding) {
  if (from_encoding.empty() || IsUtf8(value)) {
    return value;
  }
  gsize bytes = 0;
  gchar *converted = g_convert(value.data(), static_cast<gssize>(value.size()), "UTF-8", from_encoding.c_str(), nullptr, &bytes, nullptr);
  if (!converted) {
    return value;
  }
  std::string result(converted, bytes);
  g_free(converted);
  return result;
}
