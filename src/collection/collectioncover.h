#ifndef STRAWBERRY_COLLECTIONCOVER_H
#define STRAWBERRY_COLLECTIONCOVER_H

#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "constants/collectionsettings.h"
#include "core/settings.h"
#include "core/song.h"

#include <string>

namespace CollectionCover {

inline constexpr int kArtHeight = 32;
inline constexpr const char *kPlaceholderIcon = "media-optical-symbolic";

inline Song RepresentativeSong(const CollectionItem *item) {
  if (!item) {
    return Song();
  }
  const SongList songs = item->Songs();
  return songs.empty() ? Song() : songs.front();
}

inline std::string CacheKey(const Song &song) {
  if (!song.art_manual().empty()) {
    return song.art_manual();
  }
  if (!song.art_automatic().empty()) {
    return song.art_automatic();
  }
  const std::string artist = song.albumartist().empty() ? song.artist() : song.albumartist();
  if (!artist.empty() || !song.album().empty()) {
    return artist + "|" + song.album();
  }
  return song.url();
}

inline std::string CacheKey(const CollectionItem *item) { return CacheKey(RepresentativeSong(item)); }

inline bool ShouldShowThumb(bool pretty_covers, CollectionItem::Type type, int container_level,
                            const CollectionGrouping::Grouping &grouping) {
  if (!pretty_covers || type != CollectionItem::Type::Container || container_level < 0) {
    return false;
  }
  return CollectionGrouping::IsAlbumGroupBy(grouping[container_level]);
}

inline bool LoadPrettyCovers() {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  return settings.BoolValue(CollectionSettings::kPrettyCovers, CollectionSettings::kDefaultPrettyCovers);
}

}  // namespace CollectionCover

#endif
