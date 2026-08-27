#ifndef STRAWBERRY_PLAYLISTLOOK_H
#define STRAWBERRY_PLAYLISTLOOK_H

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace PlaylistLook {

inline constexpr int kGlowIntensitySteps = 24;

inline int GlowPeriod() { return kGlowIntensitySteps * 2; }

inline int GlowIntervalMs() { return 1500 / kGlowIntensitySteps; }

inline int GlowFrame(int step) {
  int frame = step % GlowPeriod();
  if (frame < 0) {
    frame += GlowPeriod();
  }
  if (frame >= kGlowIntensitySteps) {
    frame = 2 * (kGlowIntensitySteps - 1) - frame + 1;
  }
  return frame;
}

inline int NextGlowStep(int step) { return (step + 1) % GlowPeriod(); }

inline int StopGlowStep() { return kGlowIntensitySteps; }

inline double GlowOverlay(int step) {
  const int frame = GlowFrame(step);
  return 0.4 - 0.6 * std::sin(static_cast<double>(frame) / static_cast<double>(kGlowIntensitySteps) * (std::acos(-1.0) / 2.0));
}

inline double GlowBackgroundAlpha(int step) { return std::clamp(0.22 - GlowOverlay(step) * 0.15, 0.08, 0.40); }

inline double GlowBarAlpha(int step) { return std::clamp(0.35 - GlowOverlay(step) * 0.25, 0.12, 0.55); }

inline bool ShouldAnimateGlow(const bool glow_enabled, const bool bars_enabled, const bool playing) {
  return glow_enabled && bars_enabled && playing;
}

// Qt PlaylistView::ReloadSettings re-reads glow/bars/alternating and rebuilds row styling.
inline bool ShouldRefreshRowsOnReload() { return true; }

inline bool ShouldRestartGlowOnReload(bool glow_enabled, bool bars_enabled, bool currently_glowing) {
  return ShouldAnimateGlow(glow_enabled, bars_enabled, currently_glowing);
}

inline std::string FormatAlpha(const double value) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%.2f", value);
  return buf;
}

inline std::string AlternatingCss(bool enabled) {
  if (!enabled) {
    return {};
  }
  return ".playlist-row.playlist-alt { background-color: alpha(currentColor, 0.06); }";
}

inline std::string GlowCss(bool enabled, int step = 0) {
  if (!enabled) {
    return {};
  }
  return ".playlist-playing.playlist-glow { background-color: alpha(@accent_bg_color, " + FormatAlpha(GlowBackgroundAlpha(step)) + "); }";
}

inline std::string BarsCss(bool enabled, double progress, int glow_step = -1) {
  if (!enabled) {
    return {};
  }
  const int pct = std::clamp(static_cast<int>(progress * 100.0), 0, 100);
  const std::string alpha = FormatAlpha(glow_step >= 0 ? GlowBarAlpha(glow_step) : 0.35);
  return ".playlist-playing.playlist-bars { background-image: linear-gradient(to right, alpha(@accent_bg_color, " + alpha + ") " +
         std::to_string(pct) + "%, transparent " + std::to_string(pct) + "%); }";
}

inline std::string UnavailableCss() {
  return ".playlist-row.playlist-unavailable, .playlist-row.playlist-unavailable label { color: #c0c0c0; }";
}

inline std::string StopAfterCss() {
  return ".playlist-row.playlist-stop-after { box-shadow: inset 3px 0 0 @destructive_color; }";
}

inline std::string CombinedCss(bool alternating, bool glow, bool bars, double progress, int glow_step = 0) {
  return AlternatingCss(alternating) + GlowCss(glow, glow_step) + BarsCss(bars, progress, glow ? glow_step : -1) + UnavailableCss() +
         StopAfterCss();
}

}  // namespace PlaylistLook

#endif
