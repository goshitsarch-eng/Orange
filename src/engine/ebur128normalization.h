#ifndef STRAWBERRY_EBUR128NORMALIZATION_H
#define STRAWBERRY_EBUR128NORMALIZATION_H

#include <cmath>
#include <optional>

namespace Ebur128Normalization {

// Qt EngineBase::Load: target_lufs - integrated_lufs (e.g. -12 → -23 is -11 dB).
inline double GainDbFromLufs(double integrated_lufs, double target_lufs) { return target_lufs - integrated_lufs; }

inline double VolumeMultiplierFromGainDb(double gain_db) { return std::pow(10.0, gain_db / 20.0); }

inline std::optional<double> NormalizingGainDb(bool enabled, const std::optional<double> &integrated_lufs, double target_lufs) {
  if (!enabled || !integrated_lufs) {
    return std::nullopt;
  }
  return GainDbFromLufs(*integrated_lufs, target_lufs);
}

inline double EffectiveGainDb(bool enabled, const std::optional<double> &integrated_lufs, double target_lufs) {
  return NormalizingGainDb(enabled, integrated_lufs, target_lufs).value_or(0.0);
}

// Qt GstEnginePipeline: GStreamer ≥ 1.24 exposes volume-full-range so EBU
// boosts are not clamped to the default volume element's 0–10 range.
inline bool VolumeFullRangeSupported(unsigned major, unsigned minor) {
  return major > 1 || (major == 1 && minor >= 24);
}

inline const char *VolumeProperty(bool full_range_supported) { return full_range_supported ? "volume-full-range" : "volume"; }

}  // namespace Ebur128Normalization

#endif  // STRAWBERRY_EBUR128NORMALIZATION_H
