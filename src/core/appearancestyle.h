#ifndef STRAWBERRY_APPEARANCESTYLE_H
#define STRAWBERRY_APPEARANCESTYLE_H

#include <string>
#include <utility>
#include <vector>

namespace AppearanceStyle {

inline const char *kDefault = "";
inline const char *kAdwaita = "adwaita";
inline const char *kAdwaitaDark = "adwaita-dark";
inline const char *kHighContrast = "highcontrast";

inline std::vector<std::pair<std::string, std::string>> Choices() {
  return {{kDefault, "Default"}, {kAdwaita, "Adwaita"}, {kAdwaitaDark, "Adwaita Dark"}, {kHighContrast, "High Contrast"}};
}

inline bool ForcesDark(const std::string &id) { return id == kAdwaitaDark; }

inline bool HasCustomPalette(const std::string &id) { return id.empty() || id == kAdwaita || id == kAdwaitaDark; }

inline std::string CssFor(const std::string &id) {
  if (id == kHighContrast) {
    return ".strawberry-main label, .strawberry-main button { font-weight: 600; }";
  }
  return {};
}

}  // namespace AppearanceStyle

#endif  // STRAWBERRY_APPEARANCESTYLE_H
