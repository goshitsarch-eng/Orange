#ifndef STRAWBERRY_GPODLOADER_H
#define STRAWBERRY_GPODLOADER_H

#include "config.h"
#include "core/song.h"

#ifdef HAVE_GPOD
#include <gpod/itdb.h>
#endif

#include <string>

namespace GPodLoader {

#ifdef HAVE_GPOD
Song SongFromTrack(Itdb_Track *track, const std::string &prefix);
void SongToTrack(const Song &song, Itdb_Track *track);
#endif

SongList LoadSongs(const std::string &mount_path);

}  // namespace GPodLoader

#endif
