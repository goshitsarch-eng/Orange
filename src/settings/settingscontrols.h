#ifndef STRAWBERRY_SETTINGSCONTROLS_H
#define STRAWBERRY_SETTINGSCONTROLS_H

#include "constants/backendsettings.h"
#include "constants/collectionsettings.h"

#include <algorithm>
#include <limits>
#include <string>

namespace SettingsControls {

struct ScaleRange {
  double min = 0.0;
  double max = 1.0;
  double step = 0.1;
};

inline double Clamp(double value, double min, double max) { return std::min(max, std::max(min, value)); }

inline int ClampInt(int value, int min, int max) { return std::min(max, std::max(min, value)); }

inline ScaleRange BufferDurationMs() { return {100.0, 60000.0, 100.0}; }
inline ScaleRange BufferWatermark() { return {0.01, 1.0, 0.01}; }
inline ScaleRange DeviceWarmupMs() { return {0.0, 5000.0, 50.0}; }
inline ScaleRange ReplayGainDb() { return {-15.0, 15.0, 0.5}; }
inline ScaleRange EbuTargetLufs() { return {-48.0, 0.0, 0.5}; }
inline ScaleRange FadeDurationMs() { return {1000.0, 10000.0, 100.0}; }
inline ScaleRange FadePauseDurationMs() { return {50.0, 10000.0, 50.0}; }
inline ScaleRange BackgroundBlur() { return {0.0, 100.0, 1.0}; }
inline ScaleRange BackgroundOpacity() { return {0.0, 100.0, 1.0}; }

inline double ApplyRange(double value, const ScaleRange &range) { return Clamp(value, range.min, range.max); }

inline const char *NormalizationChoice(bool replaygain, bool ebu) {
  if (ebu) {
    return "ebu";
  }
  if (replaygain) {
    return "rg";
  }
  return "none";
}

inline bool NormalizationUsesReplayGain(const std::string &choice) { return choice == "rg"; }
inline bool NormalizationUsesEbu(const std::string &choice) { return choice == "ebu"; }

inline bool PlaylistColorIsSystem(const std::string &color) { return color.empty(); }

inline std::string PlaylistPlayingSongColor(bool system, const std::string &custom, const char *fallback = "#6696e3") {
  if (system) {
    return {};
  }
  return custom.empty() ? (fallback ? fallback : "#6696e3") : custom;
}

struct BufferValues {
  int duration_ms = static_cast<int>(BackendSettings::kDefaultBufferDuration);
  double low_watermark = BackendSettings::kDefaultBufferLowWatermark;
  double high_watermark = BackendSettings::kDefaultBufferHighWatermark;
  int warmup_ms = BackendSettings::kDefaultDeviceWarmupDuration;
};

inline BufferValues BufferDefaults() { return {}; }

inline int IconCacheSizeMax(int unit) {
  return unit == static_cast<int>(CollectionSettings::CacheSizeUnit::MB) ? std::numeric_limits<int>::max() / 1024
                                                                        : std::numeric_limits<int>::max();
}

inline int DiskCacheSizeMax(int unit) {
  return unit == static_cast<int>(CollectionSettings::CacheSizeUnit::GB) ? 4 : std::numeric_limits<int>::max();
}

inline int ClampCacheSize(int value, int max) { return std::max(0, std::min(value, max)); }

}  // namespace SettingsControls

#endif
