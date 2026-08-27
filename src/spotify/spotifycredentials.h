#ifndef STRAWBERRY_SPOTIFYCREDENTIALS_H
#define STRAWBERRY_SPOTIFYCREDENTIALS_H

#include <glib.h>

#include <string>

namespace SpotifyCredentials {

// Qt SpotifyService (src/spotify/spotifyservice.cpp) embeds these as base64.
inline constexpr char kClientIdB64[] = "ZTZjY2Y2OTQ5NzY1NGE3NThjOTAxNWViYzdiMWQzMTc=";
inline constexpr char kClientSecretB64[] = "N2ZlMDMxODk1NTBlNDE3ZGI1ZWQ1MzE3ZGZlZmU2MTE=";
inline constexpr char kAuthorizeUrl[] = "https://accounts.spotify.com/authorize";
inline constexpr char kTokenUrl[] = "https://accounts.spotify.com/api/token";
// Qt kOAuthRedirectUrl with random_port = false.
inline constexpr char kRedirectUri[] = "http://127.0.0.1:63111";
inline constexpr guint16 kRedirectPort = 63111;

inline std::string DecodeB64(const char *b64) {
  gsize len = 0;
  gchar *raw = reinterpret_cast<gchar *>(g_base64_decode(b64, &len));
  std::string value(raw ? raw : "", raw ? len : 0);
  g_free(raw);
  return value;
}

inline std::string DefaultClientId() { return DecodeB64(kClientIdB64); }

inline std::string DefaultClientSecret() { return DecodeB64(kClientSecretB64); }

inline std::string EffectiveClientId(const std::string &configured) { return configured.empty() ? DefaultClientId() : configured; }

inline std::string EffectiveClientSecret(const std::string &configured) {
  return configured.empty() ? DefaultClientSecret() : configured;
}

inline bool UsesEmbedded(const std::string &configured_id) { return configured_id.empty(); }

inline bool ShouldOpenBrowser(const std::string &configured_id) { return !EffectiveClientId(configured_id).empty(); }

inline std::string RedirectUri() { return kRedirectUri; }

}  // namespace SpotifyCredentials

#endif
