#include "lyrics/elyricsnetlyricsprovider.h"

#include <cctype>

ElyricsNetLyricsProvider::ElyricsNetLyricsProvider()
    : HtmlLyricsProvider("elyrics.net", "<div[^>]*>", "</div>", "<div id='inlyr'", false) {}

std::string ElyricsNetLyricsProvider::UrlFor(const Song &song) const {
  const std::string artist = SlugElyrics(song.artist());
  const char first = artist.empty() ? 'a' : artist[0];
  return "https://www.elyrics.net/read/" + std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(first)))) + "/" + artist +
         "-lyrics/" + SlugElyrics(song.title()) + "-lyrics.html";
}
