#ifndef STRAWBERRY_CDDATEXT_H
#define STRAWBERRY_CDDATEXT_H

#include "core/song.h"

#include <string>

namespace CddaText {

inline bool HasValue(const std::string &value) { return !value.empty(); }

inline bool TitleIsGeneric(const std::string &title, int track) {
  return title.empty() || title == "Track " + std::to_string(track);
}

inline void Apply(Song *song, const std::string &album, const std::string &album_artist, const std::string &title,
                  const std::string &artist) {
  if (!song) {
    return;
  }
  if (HasValue(album)) {
    song->set_album(album);
  }
  if (HasValue(album_artist)) {
    song->set_albumartist(album_artist);
    if (song->artist().empty()) {
      song->set_artist(album_artist);
    }
  }
  if (HasValue(title)) {
    song->set_title(title);
  }
  if (HasValue(artist)) {
    song->set_artist(artist);
  }
}

inline bool HasCompleteTitles(const SongList &songs) {
  if (songs.empty()) {
    return false;
  }
  for (const Song &song : songs) {
    if (TitleIsGeneric(song.title(), song.track())) {
      return false;
    }
  }
  return true;
}

}  // namespace CddaText

#endif
