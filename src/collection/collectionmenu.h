#ifndef STRAWBERRY_COLLECTIONMENU_H
#define STRAWBERRY_COLLECTIONMENU_H

#include "core/song.h"

#include <cstring>
#include <string>
#include <vector>

namespace CollectionMenu {

enum class Action {
  Append,
  Replace,
  New,
  Enqueue,
  EnqueueNext,
  SearchForThis,
  Organize,
  CopyToDevice,
  DeleteFiles,
  EditTrack,
  EditTracks,
  Browse,
  Rescan,
  VariousOn,
  VariousOff,
  GroupBy
};

struct Item {
  const char *id = "";
  Action action = Action::Append;
};

struct SelectionState {
  int selected = 0;
  int editable = 0;
  bool delete_allowed = false;
  bool devices_connected = false;
};

inline std::vector<Item> Items() {
  return {
      {"append", Action::Append},
      {"replace", Action::Replace},
      {"new", Action::New},
      {"enqueue", Action::Enqueue},
      {"enqueue-next", Action::EnqueueNext},
      {"search", Action::SearchForThis},
      {"organize", Action::Organize},
      {"copy-device", Action::CopyToDevice},
      {"delete", Action::DeleteFiles},
      {"edit-track", Action::EditTrack},
      {"edit-tracks", Action::EditTracks},
      {"browse", Action::Browse},
      {"rescan", Action::Rescan},
      {"various-on", Action::VariousOn},
      {"various-off", Action::VariousOff},
      {"group-by", Action::GroupBy},
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

inline SelectionState Analyze(const SongList &songs, bool delete_allowed = false, bool devices_connected = false) {
  SelectionState state;
  state.selected = static_cast<int>(songs.size());
  state.delete_allowed = delete_allowed;
  state.devices_connected = devices_connected;
  for (const Song &song : songs) {
    if (song.IsEditable()) {
      ++state.editable;
    }
  }
  return state;
}

inline bool AllEditable(const SelectionState &state) { return state.selected > 0 && state.selected == state.editable; }

inline bool IncludeItem(Action action, const SelectionState &state) {
  switch (action) {
    case Action::GroupBy:
      return true;
    case Action::Append:
    case Action::Replace:
    case Action::New:
    case Action::Enqueue:
    case Action::EnqueueNext:
    case Action::SearchForThis:
    case Action::Browse:
    case Action::VariousOn:
    case Action::VariousOff:
      return state.selected > 0;
    case Action::Organize:
    case Action::CopyToDevice:
      return AllEditable(state);
    case Action::DeleteFiles:
      return state.selected > 0 && state.delete_allowed;
    case Action::EditTrack:
      return state.editable == 1;
    case Action::EditTracks:
      return state.editable > 1;
    case Action::Rescan:
      return state.editable > 0;
  }
  return false;
}

inline std::vector<Item> VisibleItems(const SelectionState &state) {
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (IncludeItem(item.action, state)) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline bool Contains(const std::vector<Item> &items, Action action) {
  for (const Item &item : items) {
    if (item.action == action) {
      return true;
    }
  }
  return false;
}

inline bool CopyToDeviceEnabled(const SelectionState &state) { return AllEditable(state) && state.devices_connected; }

// Qt CollectionView::contextMenuEvent always embeds filter_widget_->menu() as Display options.
inline bool DisplayOptionsEnabled() { return true; }

inline const char *DisplayOptionsLabel() { return "Display options"; }

inline std::string LabelFor(Action action) {
  switch (action) {
    case Action::Append:
      return "Append to current playlist";
    case Action::Replace:
      return "Replace current playlist";
    case Action::New:
      return "Open in new playlist";
    case Action::Enqueue:
      return "Queue track";
    case Action::EnqueueNext:
      return "Queue to play next";
    case Action::SearchForThis:
      return "Search for this";
    case Action::Organize:
      return "Organize files...";
    case Action::CopyToDevice:
      return "Copy to device...";
    case Action::DeleteFiles:
      return "Delete from disk...";
    case Action::EditTrack:
      return "Edit track information...";
    case Action::EditTracks:
      return "Edit tracks information...";
    case Action::Browse:
      return "Show in file browser...";
    case Action::Rescan:
      return "Rescan song(s)";
    case Action::VariousOn:
      return "Show in various artists";
    case Action::VariousOff:
      return "Don't show in various artists";
    case Action::GroupBy:
      return "Group by";
  }
  return {};
}

inline const char *WinAction(Action action) {
  switch (action) {
    case Action::Append:
      return "win.collection-append";
    case Action::Replace:
      return "win.collection-replace";
    case Action::New:
      return "win.collection-new";
    case Action::Enqueue:
      return "win.collection-enqueue";
    case Action::EnqueueNext:
      return "win.collection-enqueue-next";
    case Action::SearchForThis:
      return "win.collection-search";
    case Action::Organize:
      return "win.collection-organize";
    case Action::CopyToDevice:
      return "win.collection-copy-device";
    case Action::DeleteFiles:
      return "win.collection-delete";
    case Action::EditTrack:
    case Action::EditTracks:
      return "win.collection-edittag";
    case Action::Browse:
      return "win.collection-browse";
    case Action::Rescan:
      return "win.collection-rescan";
    case Action::VariousOn:
      return "win.collection-various-on";
    case Action::VariousOff:
      return "win.collection-various-off";
    case Action::GroupBy:
      return "";
  }
  return "win.collection-append";
}

}  // namespace CollectionMenu

#endif
