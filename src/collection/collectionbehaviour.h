#ifndef STRAWBERRY_COLLECTIONBEHAVIOUR_H
#define STRAWBERRY_COLLECTIONBEHAVIOUR_H

#include "collection/collectiongrouping.h"
#include "collection/collectionitem.h"
#include "constants/behavioursettings.h"
#include "core/song.h"

#include <string>

namespace CollectionBehaviour {

enum class Destination { Current, New };

enum class QueueMode { None, Append, Next };

struct Plan {
  Destination destination = Destination::Current;
  QueueMode queue = QueueMode::None;
  bool clear_current = false;
  bool should_play = false;
};

bool ShouldPlay(BehaviourSettings::PlayBehaviour mode, bool engine_stopped);

Plan FromDoubleClick(BehaviourSettings::AddBehaviour add, BehaviourSettings::PlayBehaviour play, bool engine_stopped);
Plan Append(BehaviourSettings::PlayBehaviour menu_play, bool engine_stopped);
Plan Enqueue();
Plan EnqueueNext();
Plan OpenInNew(BehaviourSettings::PlayBehaviour menu_play, bool engine_stopped);
Plan Replace(BehaviourSettings::PlayBehaviour menu_play, bool engine_stopped);

SongList UniqueByUrl(const SongList &songs);
std::string NewPlaylistName(const SongList &songs);
std::string SearchQuery(const CollectionItem *item, const CollectionGrouping::Grouping &grouping);

// Qt MainWindow::ShowInCollection uses the first local collection song as artist:X album:Y (unquoted).
inline Song FirstCollectionSong(const SongList &songs) {
  for (const Song &song : songs) {
    if (song.is_collection_song()) {
      return song;
    }
  }
  return Song();
}

inline std::string ShowInCollectionQuery(const Song &song) {
  if (!song.is_collection_song()) {
    return {};
  }
  return "artist:" + song.artist() + " album:" + song.album();
}

inline std::string ShowInCollectionQuery(const SongList &songs) { return ShowInCollectionQuery(FirstCollectionSong(songs)); }

}  // namespace CollectionBehaviour

#endif
