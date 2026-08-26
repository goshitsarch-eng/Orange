#ifndef STRAWBERRY_MOODBARSTYLE_H
#define STRAWBERRY_MOODBARSTYLE_H

#include "constants/moodbarsettings.h"
#include "utilities/colorutils.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace MoodbarStyle {

struct Hsv {
  int h = 0;
  int s = 0;
  int v = 0;
};

struct StyleProperties {
  int threshold = 0;
  int range_start = 0;
  int range_delta = 0;
  int sat = 100;
  int val = 100;
};

inline ColorUtils::Rgb DefaultHighlight() { return {53, 132, 228}; }

inline MoodbarSettings::Style ClampStyle(int value) {
  if (value < 0 || value >= static_cast<int>(MoodbarSettings::Style::StyleCount)) {
    return MoodbarSettings::kDefaultStyle;
  }
  return static_cast<MoodbarSettings::Style>(value);
}

inline const char *StyleName(MoodbarSettings::Style style) {
  switch (style) {
    case MoodbarSettings::Style::Angry:
      return "Angry";
    case MoodbarSettings::Style::Frozen:
      return "Frozen";
    case MoodbarSettings::Style::Happy:
      return "Happy";
    case MoodbarSettings::Style::SystemPalette:
      return "System colors";
    case MoodbarSettings::Style::Normal:
    default:
      return "Normal";
  }
}

inline Hsv RgbToHsv(int r, int g, int b) {
  const int max = std::max(r, std::max(g, b));
  const int min = std::min(r, std::min(g, b));
  Hsv hsv;
  hsv.v = max;
  if (max == 0) {
    return hsv;
  }
  hsv.s = (max - min) * 255 / max;
  if (max == min) {
    hsv.h = 0;
    return hsv;
  }
  const int delta = max - min;
  if (max == r) {
    hsv.h = 60 * (g - b) / delta;
  } else if (max == g) {
    hsv.h = 120 + 60 * (b - r) / delta;
  } else {
    hsv.h = 240 + 60 * (r - g) / delta;
  }
  if (hsv.h < 0) {
    hsv.h += 360;
  }
  return hsv;
}

inline ColorUtils::Rgb HsvToRgb(int h, int s, int v) {
  h = std::clamp(h, 0, 359);
  s = std::clamp(s, 0, 255);
  v = std::clamp(v, 0, 255);
  if (s == 0) {
    return {v, v, v};
  }
  const int region = h / 60;
  const int remainder = (h - region * 60) * 255 / 60;
  const int p = v * (255 - s) / 255;
  const int q = v * (255 - s * remainder / 255) / 255;
  const int t = v * (255 - s * (255 - remainder) / 255) / 255;
  switch (region) {
    case 0:
      return {v, t, p};
    case 1:
      return {q, v, p};
    case 2:
      return {p, v, t};
    case 3:
      return {p, q, v};
    case 4:
      return {t, p, v};
    default:
      return {v, p, q};
  }
}

inline StyleProperties PropertiesFor(MoodbarSettings::Style style, int samples, ColorUtils::Rgb highlight = DefaultHighlight()) {
  StyleProperties properties;
  switch (style) {
    case MoodbarSettings::Style::Angry:
      properties = {samples / 360 * 9, 45, -45, 200, 100};
      break;
    case MoodbarSettings::Style::Frozen:
      properties = {samples / 360 * 1, 140, 160, 50, 100};
      break;
    case MoodbarSettings::Style::Happy:
      properties = {samples / 360 * 2, 0, 359, 150, 250};
      break;
    case MoodbarSettings::Style::SystemPalette: {
      const Hsv hsv = RgbToHsv(highlight.r, highlight.g, highlight.b);
      properties.threshold = samples / 360 * 3;
      properties.range_start = (hsv.h - 20 + 360) % 360;
      properties.range_delta = 20;
      properties.sat = hsv.s;
      properties.val = hsv.v / 2;
      break;
    }
    case MoodbarSettings::Style::Normal:
    default:
      properties = {samples / 360 * 3, 0, 359, 100, 100};
      break;
  }
  return properties;
}

inline std::vector<uint8_t> Apply(const std::vector<uint8_t> &mood, MoodbarSettings::Style style,
                                  ColorUtils::Rgb highlight = DefaultHighlight()) {
  const int samples = static_cast<int>(mood.size() / 3);
  if (samples <= 0) {
    return {};
  }
  const StyleProperties properties = PropertiesFor(style, samples, highlight);
  int hue_distribution[360];
  std::memset(hue_distribution, 0, sizeof(hue_distribution));
  std::vector<Hsv> colors;
  colors.reserve(static_cast<size_t>(samples));
  int total = 0;
  for (int i = 0; i < samples; ++i) {
    const Hsv hsv = RgbToHsv(mood[static_cast<size_t>(i) * 3], mood[static_cast<size_t>(i) * 3 + 1], mood[static_cast<size_t>(i) * 3 + 2]);
    colors.push_back(hsv);
    const int hue = std::max(0, hsv.h);
    if (hue_distribution[hue]++ == properties.threshold) {
      ++total;
    }
  }
  total = std::max(total, 1);
  for (int i = 0, n = 0; i < 360; ++i) {
    hue_distribution[i] = ((hue_distribution[i] > properties.threshold ? n++ : n) * properties.range_delta / total + properties.range_start) % 360;
  }
  std::vector<uint8_t> styled;
  styled.reserve(static_cast<size_t>(samples) * 3);
  for (const Hsv &hsv : colors) {
    const int hue = std::max(0, hsv.h);
    const ColorUtils::Rgb rgb = HsvToRgb(hue_distribution[hue], hsv.s * properties.sat / 100, hsv.v * properties.val / 100);
    styled.push_back(static_cast<uint8_t>(std::clamp(rgb.r, 0, 255)));
    styled.push_back(static_cast<uint8_t>(std::clamp(rgb.g, 0, 255)));
    styled.push_back(static_cast<uint8_t>(std::clamp(rgb.b, 0, 255)));
  }
  return styled;
}

}  // namespace MoodbarStyle

#endif
