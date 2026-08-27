#ifndef STRAWBERRY_SONGLOADEFFECTIVE_H
#define STRAWBERRY_SONGLOADEFFECTIVE_H

#include "core/song.h"

namespace SongLoadEffective {

// Qt SongLoader::EffectiveSongLoad: skip a second TagLib pass when ReadFile already succeeded.
inline bool AlreadyLoaded(const Song &song) {
  return song.init_from_file() && song.filetype() != Song::FileType::Unknown;
}

}  // namespace SongLoadEffective

#endif
