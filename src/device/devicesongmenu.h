#ifndef STRAWBERRY_DEVICESONGMENU_H
#define STRAWBERRY_DEVICESONGMENU_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "core/song.h"
#include "device/devicecopy.h"

#include <cstring>
#include <vector>

namespace DeviceSongMenu {

enum class Action { Append, Replace, New, Copy, Delete };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::Append;
};

inline std::vector<Item> Items() {
  return {
      {"Append to current playlist", "append", Action::Append},
      {"Replace current playlist", "replace", Action::Replace},
      {"Open in new playlist", "new", Action::New},
      {"Copy to collection…", "copy", Action::Copy},
      {"Delete from device…", "delete", Action::Delete},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline bool IsPlaylistAction(Action action) {
  return action == Action::Append || action == Action::Replace || action == Action::New;
}

inline bool IncludeItem(const Item &item, bool can_copy) { return item.action != Action::Copy || can_copy; }

inline std::vector<Item> VisibleItems(const SongList &songs) {
  const bool can_copy = DeviceCopy::CanCopyToCollection(songs);
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (IncludeItem(item, can_copy)) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Append;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Append;
}

inline CollectionBehaviour::Plan PlanFor(Action action, BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  switch (action) {
    case Action::Replace:
      return CollectionBehaviour::Replace(play, engine_stopped);
    case Action::New:
      return CollectionBehaviour::OpenInNew(play, engine_stopped);
    case Action::Append:
    default:
      return CollectionBehaviour::Append(play, engine_stopped);
  }
}

}  // namespace DeviceSongMenu

#endif
