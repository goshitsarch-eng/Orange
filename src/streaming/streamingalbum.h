#ifndef STRAWBERRY_STREAMINGALBUM_H
#define STRAWBERRY_STREAMINGALBUM_H

#include "core/song.h"

#include <string>

namespace StreamingAlbum {

inline std::string EffectiveTitle(const Song &album) { return !album.album().empty() ? album.album() : album.title(); }

inline void FillFromParent(Song *song, const Song &album) {
  if (!song) {
    return;
  }
  const std::string name = EffectiveTitle(album);
  if (song->album().empty() && !name.empty()) {
    song->set_album(name);
  }
  if (song->album_id().empty() && !album.album_id().empty()) {
    song->set_album_id(album.album_id());
  }
  if (song->artist().empty() && !album.artist().empty()) {
    song->set_artist(album.artist());
  }
  if (song->albumartist().empty() && !album.albumartist().empty()) {
    song->set_albumartist(album.albumartist());
  }
  if (song->artist_id().empty() && !album.artist_id().empty()) {
    song->set_artist_id(album.artist_id());
  }
  if (song->art_automatic().empty() && !album.art_automatic().empty()) {
    song->set_art_automatic(album.art_automatic());
  }
  if (song->genre().empty() && !album.genre().empty()) {
    song->set_genre(album.genre());
  }
  if (song->year() <= 0 && album.year() > 0) {
    song->set_year(album.year());
  }
}

inline void ApplyParent(SongList &songs, const Song &album) {
  for (Song &song : songs) {
    FillFromParent(&song, album);
  }
}

}  // namespace StreamingAlbum

#endif
