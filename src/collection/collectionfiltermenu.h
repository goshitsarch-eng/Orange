#ifndef STRAWBERRY_COLLECTIONFILTERMENU_H
#define STRAWBERRY_COLLECTIONFILTERMENU_H

#include "collection/collectiongrouping.h"

#include <string>
#include <vector>

namespace CollectionFilterMenu {

enum class DelayBehaviour { AlwaysInstant, DelayedOnLargeLibraries, AlwaysDelayed };

enum class ActionKind { Preset, Saved, Advanced, Save, Manage, Configure };

inline constexpr int kFilterDelayMs = 500;
inline constexpr int kLargeLibrarySongs = 100000;

inline bool ShouldDelay(DelayBehaviour behaviour, int text_length, int song_count) {
  if (behaviour == DelayBehaviour::AlwaysInstant) {
    return false;
  }
  if (behaviour == DelayBehaviour::AlwaysDelayed) {
    return true;
  }
  return text_length > 0 && text_length < 3 && song_count >= kLargeLibrarySongs;
}

struct Preset {
  const char *label = "";
  CollectionGrouping::Grouping grouping;
  bool advanced = false;
};

inline std::vector<Preset> BuiltinPresets() {
  using G = CollectionGrouping::GroupBy;
  return {
      {"Group by Album artist/Album", {G::AlbumArtist, G::Album, G::None}},
      {"Group by Album artist/Album - Disc", {G::AlbumArtist, G::AlbumDisc, G::None}},
      {"Group by Album artist/Year - Album", {G::AlbumArtist, G::YearAlbum, G::None}},
      {"Group by Album artist/Year - Album - Disc", {G::AlbumArtist, G::YearAlbumDisc, G::None}},
      {"Group by Artist/Album", {G::Artist, G::Album, G::None}},
      {"Group by Artist/Album - Disc", {G::Artist, G::AlbumDisc, G::None}},
      {"Group by Artist/Year - Album", {G::Artist, G::YearAlbum, G::None}},
      {"Group by Artist/Year - Album - Disc", {G::Artist, G::YearAlbumDisc, G::None}},
      {"Group by Genre/Album artist/Album", {G::Genre, G::AlbumArtist, G::Album}},
      {"Group by Genre/Artist/Album", {G::Genre, G::Artist, G::Album}},
      {"Group by Album Artist", {G::AlbumArtist, G::None, G::None}},
      {"Group by Artist", {G::Artist, G::None, G::None}},
      {"Group by Album", {G::Album, G::None, G::None}},
      {"Group by Genre/Album", {G::Genre, G::Album, G::None}},
      {"Advanced grouping...", {}, true},
  };
}

inline int MatchingPresetIndex(const CollectionGrouping::Grouping &current, const std::vector<Preset> &presets) {
  for (size_t i = 0; i < presets.size(); ++i) {
    if (!presets[i].advanced && presets[i].grouping == current) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

inline bool IsAdvancedAction(const Preset &preset) { return preset.advanced; }

}  // namespace CollectionFilterMenu

#endif
