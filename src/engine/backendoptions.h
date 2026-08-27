#ifndef STRAWBERRY_BACKENDOPTIONS_H
#define STRAWBERRY_BACKENDOPTIONS_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace BackendOptions {

inline constexpr int64_t kNsecPerMsec = 1000000LL;

inline const char *PlaybinFactory(bool playbin3) { return playbin3 ? "playbin3" : "playbin"; }

inline bool SameAlbum(const std::string &current_album, const std::string &next_album) {
  return !current_album.empty() && current_album == next_album;
}

inline bool SuppressSameAlbumCrossfade(bool auto_change, bool same_album, bool no_crossfade_same_album) {
  return auto_change && same_album && no_crossfade_same_album;
}

inline bool AllowAutoCrossfade(bool autocrossfade, bool no_crossfade_same_album, const std::string &current_album,
                               const std::string &next_album, bool same_album_flag = false) {
  if (!autocrossfade) {
    return false;
  }
  if (no_crossfade_same_album && (same_album_flag || SameAlbum(current_album, next_album))) {
    return false;
  }
  return true;
}

inline int FadeDurationMs(bool enabled, int duration_ms, int fallback_ms) {
  if (!enabled) {
    return 0;
  }
  return std::max(50, duration_ms > 0 ? duration_ms : fallback_ms);
}

inline int64_t BufferDurationNanosec(int64_t duration_ms) { return std::max<int64_t>(0, duration_ms) * kNsecPerMsec; }

inline double ClampWatermark(double value) { return std::clamp(value, 0.0, 1.0); }

inline double VolumeFraction(unsigned percent, bool exponential) {
  percent = std::min(percent, 100u);
  if (!exponential) {
    return static_cast<double>(percent) / 100.0;
  }
  if (percent == 0) {
    return 0.0;
  }
  if (percent >= 100) {
    return 1.0;
  }
  return std::pow(10.0, (static_cast<double>(percent) - 100.0) / 40.0);
}

inline int EffectiveChannels(bool enabled, int channels) { return (enabled && channels > 0) ? channels : 0; }

inline const char *SoupForceHttp1(bool http2_enabled) { return http2_enabled ? "" : "1"; }

inline int WarmupMs(bool first_pipeline, int duration_ms) { return first_pipeline ? std::max(0, duration_ms) : 0; }

}  // namespace BackendOptions

#endif
