#include "lyrics/azlyricscomlyricsprovider.h"

AzLyricsComLyricsProvider::AzLyricsComLyricsProvider()
    : HtmlLyricsProvider("azlyrics.com", "<div>", "</div>",
                         "<!-- Usage of azlyrics.com content by any third-party lyrics provider is prohibited by our licensing agreement. Sorry about that. -->",
                         false) {}

std::string AzLyricsComLyricsProvider::UrlFor(const Song &song) const {
  return "https://www.azlyrics.com/lyrics/" + SlugAzLyrics(song.artist()) + "/" + SlugAzLyrics(song.title()) + ".html";
}
