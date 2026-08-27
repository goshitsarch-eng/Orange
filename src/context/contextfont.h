#ifndef STRAWBERRY_CONTEXTFONT_H
#define STRAWBERRY_CONTEXTFONT_H

#include "constants/contextsettings.h"
#include "utilities/fontutils.h"

#include <string>

namespace ContextFont {

inline FontUtils::Font Load(const std::string &stored, int size_fallback,
                            const char *family_fallback = ContextSettings::kDefaultFontFamily) {
  FontUtils::Font font = FontUtils::Parse(stored.empty() ? family_fallback : stored);
  if (font.family.empty()) {
    font.family = family_fallback ? family_fallback : "Sans";
  }
  if (!FontUtils::HasExplicitSize(stored)) {
    font.size_pt = std::max(1, size_fallback);
  }
  return font;
}

inline std::string CssRule(const char *selector, const FontUtils::Font &font) {
  return std::string(selector ? selector : "") + " { font: " + FontUtils::ToCss(font) + "; }";
}

}  // namespace ContextFont

#endif
