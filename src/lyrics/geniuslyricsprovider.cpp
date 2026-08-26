#include "lyrics/geniuslyricsprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

GeniusLyricsProvider::GeniusLyricsProvider()
    : HtmlLyricsProvider("Genius", "<div[^>]*>", "</div>", "data-lyrics-container", true) {}

std::string GeniusLyricsProvider::UrlFor(const Song &song) const {
  return "https://genius.com/" + SlugDashed(song.artist()) + "-" + SlugDashed(song.title()) + "-lyrics";
}

void GeniusLyricsProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.artist().empty() || song.title().empty()) {
    callback({}, "No artist or title");
    return;
  }
  const std::string page_url = UrlFor(song);
  network->Get(page_url, [this, song, network, callback](const NetworkAccessManager::Response &response) {
    if (response.ok()) {
      const std::string lyrics = ParseLyricsFromHTML(response.body, start_tag_, end_tag_, lyrics_start_, multiple_);
      if (!lyrics.empty()) {
        callback(lyrics, {});
        return;
      }
    }
    const std::string search = "https://genius.com/api/search/song?q=" + StrUtils::UriEscape(song.artist() + " " + song.title());
    network->Get(search, [this, network, callback](const NetworkAccessManager::Response &search_response) {
      if (!search_response.ok()) {
        callback({}, search_response.error.empty() ? "Genius search failed" : search_response.error);
        return;
      }
      const std::string url = JsonUtils::FindStringByKeys(search_response.body, {"url", "path"});
      if (url.empty()) {
        callback({}, "No Genius result");
        return;
      }
      std::string page = url;
      if (StrUtils::StartsWith(page, "/")) {
        page = "https://genius.com" + page;
      }
      network->Get(page, [this, callback](const NetworkAccessManager::Response &page_response) {
        if (!page_response.ok()) {
          callback({}, page_response.error.empty() ? "Genius lyrics request failed" : page_response.error);
          return;
        }
        const std::string lyrics = ParseLyricsFromHTML(page_response.body, start_tag_, end_tag_, lyrics_start_, multiple_);
        if (lyrics.empty()) {
          callback({}, "No lyrics in Genius response");
          return;
        }
        callback(lyrics, {});
      });
    });
  });
}
