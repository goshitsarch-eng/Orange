#ifndef STRAWBERRY_DEVICECOPY_H
#define STRAWBERRY_DEVICECOPY_H

#include "core/song.h"
#include "organize/organizedialog.h"

namespace DeviceCopy {

inline bool CanCopyToCollection(const SongList &songs) { return !songs.empty(); }

inline OrganizeDialog::Request CollectionRequest(const SongList &songs) {
  OrganizeDialog::Request request;
  request.songs = songs;
  request.move = false;
  return request;
}

}  // namespace DeviceCopy

#endif
