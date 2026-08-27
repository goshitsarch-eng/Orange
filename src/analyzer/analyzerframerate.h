#ifndef STRAWBERRY_ANALYZERFRAMERATE_H
#define STRAWBERRY_ANALYZERFRAMERATE_H

#include "constants/analyzersettings.h"

#include <cmath>
#include <string>
#include <vector>

namespace AnalyzerFramerate {

struct Preset {
  const char *id;
  const char *label;
  int fps;
};

inline std::vector<Preset> Presets() {
  return {
      {"low", "Low (20 fps)", AnalyzerSettings::kLowFramerate},
      {"medium", "Medium (25 fps)", AnalyzerSettings::kMediumFramerate},
      {"high", "High (30 fps)", AnalyzerSettings::kHighFramerate},
      {"super", "Super high (60 fps)", AnalyzerSettings::kSuperHighFramerate},
  };
}

inline std::string LabelFor(int fps) {
  for (const Preset &preset : Presets()) {
    if (preset.fps == fps) {
      return preset.label;
    }
  }
  return "Custom (" + std::to_string(fps) + " fps)";
}

inline int Nearest(int fps) {
  int best = AnalyzerSettings::kMediumFramerate;
  int distance = 1000;
  for (const Preset &preset : Presets()) {
    const int delta = std::abs(preset.fps - fps);
    if (delta < distance) {
      distance = delta;
      best = preset.fps;
    }
  }
  return best;
}

}  // namespace AnalyzerFramerate

#endif
