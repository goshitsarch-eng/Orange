#ifndef STRAWBERRY_FONTUTILS_H
#define STRAWBERRY_FONTUTILS_H

#include <algorithm>
#include <cstdlib>
#include <glib.h>
#include <pango/pangocairo.h>
#include <sstream>
#include <string>
#include <vector>

namespace FontUtils {

struct Font {
  std::string family = "Sans";
  int size_pt = 12;
  bool bold = false;
  bool italic = false;
};

inline bool EqualsIgnoreCase(const std::string &value, const char *expected) {
  return g_ascii_strcasecmp(value.c_str(), expected) == 0;
}

inline bool IsStyleToken(const std::string &token) {
  return EqualsIgnoreCase(token, "Bold") || EqualsIgnoreCase(token, "Italic") || EqualsIgnoreCase(token, "Oblique") ||
         EqualsIgnoreCase(token, "Light") || EqualsIgnoreCase(token, "Medium") || EqualsIgnoreCase(token, "Regular") ||
         EqualsIgnoreCase(token, "Normal");
}

inline int ParseSizeToken(const std::string &token) {
  char *end = nullptr;
  const long size = std::strtol(token.c_str(), &end, 10);
  if (!end || end == token.c_str()) {
    return 0;
  }
  if (*end == '\0' || EqualsIgnoreCase(end, "pt") || EqualsIgnoreCase(end, "px")) {
    return static_cast<int>(size);
  }
  return 0;
}

inline Font ParseQt(const std::string &value) {
  Font font;
  std::vector<std::string> fields;
  std::string field;
  std::istringstream in(value);
  while (std::getline(in, field, ',')) {
    fields.push_back(field);
  }
  if (!fields.empty() && !fields[0].empty()) {
    font.family = fields[0];
  }
  if (fields.size() > 1) {
    font.size_pt = std::max(1, std::atoi(fields[1].c_str()));
  }
  if (fields.size() > 5) {
    const int weight = std::atoi(fields[4].c_str());
    font.bold = weight <= 100 ? weight >= 75 : weight >= 700;
    font.italic = std::atoi(fields[5].c_str()) != 0;
  }
  return font;
}

inline Font ParsePango(const std::string &value) {
  Font font;
  std::vector<std::string> tokens;
  std::istringstream in(value);
  std::string token;
  while (in >> token) {
    tokens.push_back(token);
  }
  if (tokens.empty()) {
    return font;
  }
  if (const int size = ParseSizeToken(tokens.back())) {
    font.size_pt = size;
    tokens.pop_back();
  }
  std::string family;
  for (const auto &part : tokens) {
    if (EqualsIgnoreCase(part, "Bold")) {
      font.bold = true;
    } else if (EqualsIgnoreCase(part, "Italic") || EqualsIgnoreCase(part, "Oblique")) {
      font.italic = true;
    } else if (IsStyleToken(part)) {
      continue;
    } else {
      if (!family.empty()) {
        family.push_back(' ');
      }
      family += part;
    }
  }
  if (!family.empty()) {
    font.family = family;
  }
  return font;
}

inline Font Parse(const std::string &value) {
  if (value.empty()) {
    return {};
  }
  const auto first = value.find(',');
  if (first != std::string::npos && value.find(',', first + 1) != std::string::npos) {
    return ParseQt(value);
  }
  return ParsePango(value);
}

inline bool HasExplicitSize(const std::string &value) {
  if (value.empty()) {
    return false;
  }
  const auto first = value.find(',');
  if (first != std::string::npos && value.find(',', first + 1) != std::string::npos) {
    return true;
  }
  std::istringstream in(value);
  std::string token;
  std::string last;
  while (in >> token) {
    last = token;
  }
  return ParseSizeToken(last) > 0;
}

inline std::string ToPango(const Font &font) {
  std::string out = font.family.empty() ? "Sans" : font.family;
  if (font.bold) {
    out += " Bold";
  }
  if (font.italic) {
    out += " Italic";
  }
  out += " " + std::to_string(std::max(1, font.size_pt));
  return out;
}

inline std::string ToCss(const Font &font) {
  std::string out;
  if (font.italic) {
    out += "italic ";
  }
  if (font.bold) {
    out += "bold ";
  }
  out += std::to_string(std::max(1, font.size_pt)) + "pt \"";
  out += font.family.empty() ? "Sans" : font.family;
  out += "\"";
  return out;
}

// Whether the font map actually has this family.
// A family that is only named in the settings does not exist on every machine, and asking GTK to select a
// font it cannot resolve is both wrong for the user and, on GTK 4.14, a critical from inside
// gtk_font_dialog_button_set_font_desc.
inline bool FamilyIsInstalled(const std::string &family) {
  if (family.empty()) {
    return false;
  }
  PangoFontMap *map = pango_cairo_font_map_get_default();
  if (!map) {
    return false;
  }
  PangoFontFamily **families = nullptr;
  int count = 0;
  pango_font_map_list_families(map, &families, &count);
  bool found = false;
  for (int i = 0; i < count && !found; ++i) {
    const char *name = pango_font_family_get_name(families[i]);
    found = name && g_ascii_strcasecmp(name, family.c_str()) == 0;
  }
  g_free(families);
  return found;
}

// The family to actually render with: the requested one when it exists, otherwise a generic that always
// resolves.
inline std::string ResolveInstalledFamily(const std::string &family) {
  return FamilyIsInstalled(family) ? family : std::string("Sans");
}

}  // namespace FontUtils

#endif
