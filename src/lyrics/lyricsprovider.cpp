#include "lyrics/lyricsprovider.h"

bool LyricsProvider::StartSearch(int id, const LyricsSearchRequest &request, NetworkAccessManager *network,
                                 const std::function<void(int, const LyricsSearchResults &)> &finished) {
  Song song;
  song.set_artist(request.artist.empty() ? request.albumartist : request.artist);
  song.set_albumartist(request.albumartist);
  song.set_album(request.album);
  song.set_title(request.title);
  if (!enabled()) {
    finished(id, {});
    return false;
  }
  Fetch(song, network, [this, id, finished](const std::string &lyrics, const std::string &) {
    LyricsSearchResults results;
    if (!lyrics.empty()) {
      LyricsSearchResult result;
      result.provider = name();
      result.lyrics = lyrics;
      result.score = 1.0f;
      results.push_back(result);
    }
    finished(id, results);
  });
  return true;
}
