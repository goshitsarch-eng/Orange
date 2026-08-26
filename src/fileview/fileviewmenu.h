#ifndef STRAWBERRY_FILEVIEWMENU_H
#define STRAWBERRY_FILEVIEWMENU_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "utilities/fileutils.h"

#include <cstring>
#include <string>
#include <vector>

namespace FileViewMenu {

enum class Action { Append, Replace, New, Copy, Move, Device, Delete, EditTags, Browse };

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
      {"Move to collection…", "move", Action::Move},
      {"Copy to device…", "device", Action::Device},
      {"Delete from disk…", "delete", Action::Delete},
      {"Edit track information…", "edit-tags", Action::EditTags},
      {"Show in file browser…", "browse", Action::Browse},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

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

inline bool IsPlaylistAction(Action action) {
  return action == Action::Append || action == Action::Replace || action == Action::New;
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

inline std::vector<std::string> ExpandPaths(const std::vector<std::string> &paths) {
  std::vector<std::string> files;
  for (const std::string &path : paths) {
    if (path.empty()) {
      continue;
    }
    if (FileUtils::IsDirectory(path)) {
      const std::vector<std::string> children = FileUtils::ListDirectory(path);
      for (const std::string &child : children) {
        if (!FileUtils::IsDirectory(child)) {
          files.push_back(child);
        }
      }
    } else {
      files.push_back(path);
    }
  }
  return files;
}

inline std::string BrowserPath(const std::vector<std::string> &paths) {
  if (paths.empty()) {
    return {};
  }
  return paths.front();
}

}  // namespace FileViewMenu

#endif
