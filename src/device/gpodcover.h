#ifndef STRAWBERRY_GPODCOVER_H
#define STRAWBERRY_GPODCOVER_H

#include <string>

namespace GPodCover {

// Qt GPodDevice::CopyToStorage: albumcover_ plus a cover file calls itdb_track_set_thumbnails.
inline bool ShouldSetThumbnails(bool albumcover, const std::string &cover_source) {
  return albumcover && !cover_source.empty();
}

}  // namespace GPodCover

#endif
