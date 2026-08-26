#include "covermanager/albumcovermanagerlist.h"

void AlbumCoverManagerList::SetSongs(const SongList &songs) {
  albums_.clear();
  with_cover_ = 0;
  std::string last;
  for (const Song &song : songs) {
    const std::string key = song.EffectiveAlbumartist() + "\0" + song.album();
    if (key == last || song.album().empty()) {
      continue;
    }
    last = key;
    albums_.push_back(song);
    if (!song.art_manual().empty() || !song.art_automatic().empty()) {
      ++with_cover_;
    }
  }
}
