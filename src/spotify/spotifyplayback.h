#ifndef STRAWBERRY_SPOTIFYPLAYBACK_H
#define STRAWBERRY_SPOTIFYPLAYBACK_H

#include "core/urlhandler.h"
#include "settings/streamingsettingslabels.h"
#include "streaming/streamingmetadataqueue.h"

#include <gst/gst.h>

#include <string>

namespace SpotifyPlayback {

// Qt SpotifyService::kOAuthScope plus the profile scopes the GTK settings page already requested.
inline constexpr char kOAuthScope[] =
    "user-read-private user-read-email user-follow-read user-follow-modify user-library-read user-library-modify streaming";

inline bool IsSpotifyUrl(const std::string &url) { return url.rfind("spotify:", 0) == 0; }

inline std::string TrackId(const std::string &url) { return StreamingMetadataQueue::SpotifyTrackId({}, url); }

inline std::string CanonicalPlayUrl(const std::string &url) {
  const std::string id = TrackId(url);
  if (id.empty()) {
    return url;
  }
  return "spotify://" + id;
}

inline bool PluginAvailable() {
  GstRegistry *reg = gst_registry_get();
  if (!reg) {
    return false;
  }
  if (GstPluginFeature *feature = gst_registry_lookup_feature(reg, SpotifySettingsLabels::PluginFeature())) {
    gst_object_unref(feature);
    return true;
  }
  return false;
}

inline bool UseNativePlayback(const std::string &url, bool plugin_present, bool authenticated) {
  return IsSpotifyUrl(url) && plugin_present && authenticated && !TrackId(url).empty();
}

inline bool ShouldSetAccessToken(const std::string &token, bool has_property) { return !token.empty() && has_property; }

inline std::string EffectivePlayUrl(const std::string &url, const std::string &preview_url, bool plugin_present, bool authenticated) {
  if (UseNativePlayback(url, plugin_present, authenticated)) {
    return CanonicalPlayUrl(url);
  }
  return preview_url;
}

inline UrlHandler::LoadResult NativeResult(const std::string &url) {
  UrlHandler::LoadResult result;
  result.type = UrlHandler::LoadResult::Type::TrackAvailable;
  result.media_url = url;
  result.stream_url = CanonicalPlayUrl(url);
  return result;
}

}  // namespace SpotifyPlayback

#endif
