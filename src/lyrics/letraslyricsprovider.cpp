#include "lyrics/letraslyricsprovider.h"

LetrasLyricsProvider::LetrasLyricsProvider()
    : HtmlLyricsProvider("letras.mus.br", "<div[^>]*>", "</div>", "<div class=\"lyric-original\">", false) {}

std::string LetrasLyricsProvider::UrlFor(const Song &song) const {
  return "https://www.letras.mus.br/" + SlugLetras(song.artist()) + "/" + SlugLetras(song.title()) + "/";
}

std::map<std::string, std::string> LetrasLyricsProvider::RequestHeaders() const {
  return {{"User-Agent", "Strawberry/1.2.0 (+https://www.strawberrymusicplayer.org)"}};
}
