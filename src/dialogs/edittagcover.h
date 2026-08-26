#ifndef STRAWBERRY_EDITTAGCOVER_H
#define STRAWBERRY_EDITTAGCOVER_H

#include "core/song.h"
#include "covermanager/coveroptions.h"

namespace EditTagCover {

inline bool AnySupported(const SongList &songs) {
  for (const Song &song : songs) {
    if (song.save_embedded_cover_supported()) {
      return true;
    }
  }
  return false;
}

inline bool DefaultEmbeddedChecked(const Song &song, CoverOptions::CoverType collection_save_type) {
  return (song.art_embedded() || collection_save_type == CoverOptions::CoverType::Embedded) && song.save_embedded_cover_supported();
}

inline CoverOptions::CoverType EffectiveSaveType(CoverOptions::CoverType collection_save_type, bool embedded_override) {
  return embedded_override ? CoverOptions::CoverType::Embedded : collection_save_type;
}

inline std::string ImageBytes(const std::string &image_or_path) {
  if (FileUtils::Exists(image_or_path) && FileUtils::IsFile(image_or_path)) {
    return FileUtils::ReadFile(image_or_path);
  }
  return image_or_path;
}

}  // namespace EditTagCover

#endif
