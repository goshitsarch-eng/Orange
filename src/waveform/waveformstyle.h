#ifndef STRAWBERRY_WAVEFORMSTYLE_H
#define STRAWBERRY_WAVEFORMSTYLE_H

#include "constants/waveformsettings.h"
#include "utilities/colorutils.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace WaveformStyle {

constexpr float kCurveExponent = 0.65F;

inline ColorUtils::Rgb DefaultBarColor() { return ColorUtils::RgbFromHex(WaveformSettings::kDefaultColor); }

inline ColorUtils::Rgb BarColorFromHex(const std::string &color) {
  if (color.empty()) {
    return DefaultBarColor();
  }
  return ColorUtils::RgbFromHex(color);
}

inline double ShapedAmplitude(float peak) {
  const float clamped = std::clamp(peak, 0.0f, 1.0f);
  return std::pow(static_cast<double>(clamped), static_cast<double>(kCurveExponent));
}

inline std::string HiddenSidecar(const std::string &song_path) {
  return FileUtils::Join(FileUtils::DirName(song_path), "." + FileUtils::BaseName(song_path) + ".waveform");
}

inline std::string VisibleSidecar(const std::string &song_path) {
  return FileUtils::Join(FileUtils::DirName(song_path), FileUtils::BaseName(song_path) + ".waveform");
}

inline std::vector<std::string> Sidecars(const std::string &song_path) {
  if (song_path.empty()) {
    return {};
  }
  return {HiddenSidecar(song_path), VisibleSidecar(song_path)};
}

inline std::string CacheFile(const std::string &cache_dir, const std::string &url) {
  return FileUtils::Join(cache_dir, FileUtils::BaseName(url) + ".wave");
}

}  // namespace WaveformStyle

#endif
