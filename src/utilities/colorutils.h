#ifndef STRAWBERRY_COLORUTILS_H
#define STRAWBERRY_COLORUTILS_H

#include <string>

namespace ColorUtils {

struct Rgb {
  int r = 0;
  int g = 0;
  int b = 0;
};

std::string ColorToRgba(int r, int g, int b, double a = 1.0);
std::string HexToCss(unsigned hex);
bool IsColorDark(int r, int g, int b);
unsigned ParseHex(const std::string &color);

inline Rgb RgbFromHex(const std::string &color) {
  const unsigned value = ParseHex(color);
  return {static_cast<int>((value >> 16) & 0xFF), static_cast<int>((value >> 8) & 0xFF), static_cast<int>(value & 0xFF)};
}

inline std::string HexFromRgb(Rgb rgb) {
  const unsigned hex = (static_cast<unsigned>(rgb.r & 0xFF) << 16) | (static_cast<unsigned>(rgb.g & 0xFF) << 8) |
                       static_cast<unsigned>(rgb.b & 0xFF);
  return HexToCss(hex);
}

}  // namespace ColorUtils

#endif
