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

SongList UniqueByUrl(const SongList &songs);
std::string NewPlaylistName(const SongList &songs);
std::string SearchQuery(const CollectionItem *item, const CollectionGrouping::Grouping &grouping);

}  // namespace CollectionBehaviour

#endif
