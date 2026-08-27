#ifndef STRAWBERRY_GENIUSLYRICSCREDENTIALS_H
#define STRAWBERRY_GENIUSLYRICSCREDENTIALS_H

#include <glib.h>

#include <string>

namespace GeniusLyricsCredentials {

// Qt GeniusLyricsProvider embeds these as base64. Genius pins redirect
// http://localhost:63111/ to this Client ID.
inline constexpr char kClientIdB64[] =
    "RUNTNXU4U1VyMU1KUU5hdTZySEZteUxXY2hkanFiY3lfc2JjdXBpNG5WMU9SNUg4dTBZelEtZTZCdFg2dl91SQ==";
inline constexpr char kClientSecretB64[] =
    "VE9pMU9vUjNtTXZ3eFR3YVN0QVRyUjVoUlhVWDI1Ylp5X240eEt1M0ZkYlNwRG5JUnd0LXFFbHdGZkZkRWY2VzJ1S011UnQzM3c2Y3hqY0tVZ3NGN2c=";
inline constexpr char kTokenUrl[] = "https://api.genius.com/oauth/token";
inline constexpr char kScope[] = "me";
inline constexpr bool kHideManualFields = true;

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

}  // namespace GeniusLyricsCredentials

#endif  // STRAWBERRY_GENIUSLYRICSCREDENTIALS_H
