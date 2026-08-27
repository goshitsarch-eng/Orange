#ifndef STRAWBERRY_COLLECTIONALBUMART_H
#define STRAWBERRY_COLLECTIONALBUMART_H

#include "core/song.h"

#include <string>

namespace CollectionAlbumArt {

enum class Op { Manual, Embedded, Clear, Unset };

inline bool ShouldPropagate(Song::Source source) {
  return source == Song::Source::Collection || source == Song::Source::LocalFile || source == Song::Source::Device;
}

inline bool AlbumKeyValid(const std::string &artist, const std::string &album) { return !album.empty() && !artist.empty(); }

inline bool AlbumKeyValid(const Song &song) { return AlbumKeyValid(song.EffectiveAlbumartist(), song.album()); }

}  // namespace CollectionAlbumArt

#endif  // STRAWBERRY_COLLECTIONALBUMART_H
