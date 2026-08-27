#include "lyrics/jsonlyricsprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

JsonLyricsProvider::JsonLyricsProvider(std::string name, std::string url_template)
    : name_(std::move(name)), url_template_(std::move(url_template)) {}

std::string JsonLyricsProvider::UrlFor(const Song &song) const {
  std::string url = url_template_;
  url = StrUtils::Replace(url, "{artist}", StrUtils::UriEscape(song.artist()));
  url = StrUtils::Replace(url, "{title}", StrUtils::UriEscape(song.title()));
  url = StrUtils::Replace(url, "{album}", StrUtils::UriEscape(song.album()));
  return url;
}

std::string JsonLyricsProvider::Extract(const std::string &body) const { return JsonUtils::ExtractLyrics(body); }

void JsonLyricsProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.artist().empty() || song.title().empty()) {
    callback({}, "No artist or title");
    return;
  }
  network->Get(UrlFor(song), [this, callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Lyrics request failed" : response.error);
      return;
    }
    const std::string lyrics = Extract(response.body);
    if (lyrics.empty()) {
      callback({}, "No lyrics in provider response");
      return;
    }
    callback(lyrics, {});
  });
}
