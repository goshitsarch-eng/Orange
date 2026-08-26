#ifndef STRAWBERRY_GPODDEVICE_H
#define STRAWBERRY_GPODDEVICE_H

#include "core/song.h"

#include <string>

namespace GPodDevice {

bool CopySongs(const std::string &mount_path, const SongList &songs);

}  // namespace GPodDevice

#endif
