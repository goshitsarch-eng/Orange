#ifndef STRAWBERRY_CONTEXTFONTCONTROLS_H
#define STRAWBERRY_CONTEXTFONTCONTROLS_H

#include "constants/contextsettings.h"
#include "context/contextfont.h"

#include <algorithm>
#include <string>

namespace ContextFontControls {

inline double MinPt() { return 6.0; }
inline double MaxPt() { return 32.0; }
inline double Step() { return 0.5; }

inline const char *HeadlineGroup() { return "Font for headline"; }
inline const char *NormalGroup() { return "Font for data and lyrics"; }
inline const char *FontTitle() { return "Font"; }
inline const char *SizeTitle() { return "Font size"; }
inline const char *SizeSuffix() { return " pt"; }

inline double ClampPt(double value) { return std::min(MaxPt(), std::max(MinPt(), value)); }

inline FontUtils::Font Headline(const std::string &family, double size_pt) {
  return ContextFont::Load(family, static_cast<int>(ClampPt(size_pt) + 0.5), ContextSettings::kDefaultFontFamily);
}

inline FontUtils::Font Normal(const std::string &family, double size_pt) {
  return ContextFont::Load(family, static_cast<int>(ClampPt(size_pt) + 0.5), ContextSettings::kDefaultFontFamily);
}

}  // namespace ContextFontControls

#endif
