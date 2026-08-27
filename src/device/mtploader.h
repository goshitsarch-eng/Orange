#ifndef STRAWBERRY_MTPLOADER_H
#define STRAWBERRY_MTPLOADER_H

#include "config.h"
#include "core/song.h"

#ifdef HAVE_MTP
#include <libmtp.h>
#endif

#include <string>

namespace MtpLoader {

#ifdef HAVE_MTP
Song SongFromTrack(const LIBMTP_track_t *track, const std::string &host);
void SongToTrack(const Song &song, LIBMTP_track_t *track);
#endif

SongList LoadSongs(const std::string &serial);
std::string ParseHost(const std::string &url);
uint32_t ParseItemId(const std::string &url);
std::string MakeUrl(const std::string &host, uint32_t item_id);

}  // namespace MtpLoader

#endif
