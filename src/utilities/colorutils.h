#ifndef STRAWBERRY_COLORUTILS_H
#define STRAWBERRY_COLORUTILS_H

#include <string>

namespace ColorUtils {

std::string ColorToRgba(int r, int g, int b, double a = 1.0);
std::string HexToCss(unsigned hex);
bool IsColorDark(int r, int g, int b);
unsigned ParseHex(const std::string &color);

}  // namespace ColorUtils

#endif
