#ifndef STRAWBERRY_MTPDEVICE_H
#define STRAWBERRY_MTPDEVICE_H

#include "config.h"
#include "core/song.h"

#include <string>

namespace MtpDevice {

bool CopySongs(const std::string &serial, const SongList &songs);
bool DeleteSong(const std::string &serial, const Song &song);
std::string DownloadTrack(const std::string &url);

}  // namespace MtpDevice

#endif
