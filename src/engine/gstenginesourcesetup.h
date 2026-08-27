#ifndef STRAWBERRY_GSTENGINESOURCESETUP_H
#define STRAWBERRY_GSTENGINESOURCESETUP_H

#include "version.h"

#include <string>

namespace GstSourceSetup {

// Qt GstEnginePipeline::SourceSetupCallback: "Strawberry {version}".
inline std::string UserAgentString() { return std::string("Strawberry ") + STRAWBERRY_VERSION_DISPLAY; }

inline bool ShouldSetDevice(const std::string &device) { return !device.empty(); }

// Qt always sets automatic-redirect TRUE on HTTP sources.
inline bool AutomaticRedirect() { return true; }

// Qt SourceSetupCallback only touches spotifyaudiosrc for spotify:// URLs.
inline bool IsSpotifyUrl(const std::string &url) {
  return url.rfind("spotify:", 0) == 0 || url.rfind("SPOTIFY:", 0) == 0;
}

// Qt g_object_set(source, "bitrate", 2) — gstspotify bitrate enum VeryHigh.
inline int SpotifyBitrate() { return 2; }

inline bool ShouldSetSpotifyBitrate(const std::string &url) { return IsSpotifyUrl(url); }

inline bool ShouldSetSpotifyAccessToken(const std::string &url, const std::string &token) {
  return IsSpotifyUrl(url) && !token.empty();
}

}  // namespace GstSourceSetup

#endif  // STRAWBERRY_GSTENGINESOURCESETUP_H
