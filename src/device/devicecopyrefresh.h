#ifndef STRAWBERRY_DEVICECOPYREFRESH_H
#define STRAWBERRY_DEVICECOPYREFRESH_H

#include "core/song.h"

#include <string>

namespace DeviceCopyRefresh {

// Qt MtpDevice/GPodDevice::FinishCopy writes songs_to_add_ into the device collection model.
inline bool ShouldRefreshAfterCopy(const std::string &backend, int copied) {
  return copied > 0 && !backend.empty() && backend != "cdda";
}

inline void ApplyMtpCollectionFields(Song *song) {
  if (!song) {
    return;
  }
  song->set_directory_id(1);
  song->set_artist(song->EffectiveAlbumartist());
  song->set_albumartist("");
}

inline void ApplyGPodCollectionFields(Song *song) {
  if (!song) {
    return;
  }
  song->set_directory_id(1);
}

}  // namespace DeviceCopyRefresh

#endif
