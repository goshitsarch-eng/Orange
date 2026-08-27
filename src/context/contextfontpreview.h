#ifndef STRAWBERRY_CONTEXTFONTPREVIEW_H
#define STRAWBERRY_CONTEXTFONTPREVIEW_H

#include "utilities/fontutils.h"

namespace ContextFontPreview {

inline const char *Title() { return "Preview"; }
inline const char *HeadlineSample() { return "The quick brown fox jumps over the lazy dog"; }
inline const char *NormalSample() { return "Lyrics and technical data use this font."; }

inline std::string Pango(const FontUtils::Font &font) { return FontUtils::ToPango(font); }

}  // namespace ContextFontPreview

#endif
