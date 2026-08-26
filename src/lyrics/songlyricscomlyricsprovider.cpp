#include "lyrics/songlyricscomlyricsprovider.h"

SongLyricsComLyricsProvider::SongLyricsComLyricsProvider()
    : HtmlLyricsProvider("songlyrics.com", "<div[^>]*>", "</div>", "<div id=\"songLyricsDiv\"", false) {}

std::string SongLyricsComLyricsProvider::UrlFor(const Song &song) const {
  return "https://www.songlyrics.com/" + SlugDashed(song.artist()) + "/" + SlugDashed(song.title()) + "-lyrics/";
}
