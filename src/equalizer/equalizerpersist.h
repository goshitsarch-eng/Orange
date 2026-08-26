#ifndef STRAWBERRY_EQUALIZERPERSIST_H
#define STRAWBERRY_EQUALIZERPERSIST_H

#include <algorithm>
#include <string>
#include <vector>

namespace EqualizerPersist {

inline constexpr char kSettingsGroup[] = "Equalizer";
inline constexpr char kSelectedPreset[] = "selected_preset";
inline constexpr char kEnableStereoBalancer[] = "enable_stereo_balancer";
inline constexpr char kStereoBalance[] = "stereo_balance";
inline constexpr char kDefaultPreset[] = "Custom";

inline int ClampBalance(int value) { return std::clamp(value, -100, 100); }

inline int EffectiveBalance(bool balancer_enabled, int balance) { return balancer_enabled ? ClampBalance(balance) : 0; }

inline float EffectiveBalanceFraction(bool balancer_enabled, int balance) {
  return static_cast<float>(EffectiveBalance(balancer_enabled, balance)) / 100.0f;
}

inline std::vector<int> EffectiveGains(bool equalizer_enabled, const std::vector<int> &gains) {
  return equalizer_enabled ? gains : std::vector<int>(10, 0);
}

inline int EffectivePreamp(bool equalizer_enabled, int preamp) { return equalizer_enabled ? preamp : 0; }

inline std::string PresetOrDefault(const std::string &name) { return name.empty() ? std::string(kDefaultPreset) : name; }

inline bool MigrateBalancerEnabled(bool has_key, bool stored, int legacy_balance) {
  if (has_key) {
    return stored;
  }
  return legacy_balance != 0;
}

inline std::string DbLabel(int gain) { return std::to_string(gain) + " dB"; }

}  // namespace EqualizerPersist

#endif
