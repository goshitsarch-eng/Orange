#include "utilities/colorutils.h"

#include <cstdio>
#include <cstdlib>

namespace ColorUtils {

std::string ColorToRgba(int r, int g, int b, double a) {
  char buf[48];
  std::snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%.3f)", r, g, b, a);
  return buf;
}

bool IsColorDark(int r, int g, int b) { return (0.299 * r + 0.587 * g + 0.114 * b) < 128.0; }

unsigned ParseHex(const std::string &color) {
  std::string hex = color;
  if (!hex.empty() && hex[0] == '#') {
    hex.erase(hex.begin());
  }
  return static_cast<unsigned>(std::strtoul(hex.c_str(), nullptr, 16));
}

}  // namespace ColorUtils
