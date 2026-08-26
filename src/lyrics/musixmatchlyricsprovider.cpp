#include "lyrics/musixmatchlyricsprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>

namespace {
constexpr char kApiKeyB64[] = "Y2FhMDRlN2Y4OWE5OTIxYmZlOGMzOWQzOGI3ZGU4MjE=";

std::string ApiKey() {
  gsize length = 0;
  gchar *decoded = reinterpret_cast<gchar *>(g_base64_decode(kApiKeyB64, &length));
  std::string key(decoded ? decoded : "", length);
  g_free(decoded);
  return key;
}
}  // namespace

MusixmatchLyricsProvider::MusixmatchLyricsProvider() : JsonLyricsProvider("Musixmatch", {}) {}

std::string MusixmatchLyricsProvider::UrlFor(const Song &song) const {
  return "https://api.musixmatch.com/ws/1.1/matcher.lyrics.get?apikey=" + ApiKey() + "&q_artist=" + StrUtils::UriEscape(song.artist()) +
         "&q_track=" + StrUtils::UriEscape(song.title());
}

std::string MusixmatchLyricsProvider::Extract(const std::string &body) const {
  std::string lyrics = JsonUtils::GetString(body, {"message", "body", "lyrics", "lyrics_body"});
  if (lyrics.empty()) {
    lyrics = JsonUtils::FindStringByKeys(body, {"lyrics_body", "plainLyrics", "lyrics"});
  }
  const auto disclaimer = lyrics.find("*******");
  if (disclaimer != std::string::npos) {
    lyrics = lyrics.substr(0, disclaimer);
  }
  return StrUtils::Trim(lyrics);
}
