#ifndef STRAWBERRY_PLAYLISTMENU_H
#define STRAWBERRY_PLAYLISTMENU_H

#include "core/playeritemoptions.h"
#include "core/song.h"
#include "playlist/playlistdelegates.h"

#include <cstring>
#include <string>
#include <vector>

namespace PlaylistMenu {

enum class Action {
  Play,
  Stop,
  StopAfter,
  Queue,
  QueueNext,
  Skip,
  AddToPlaylist,
  Remove,
  EditTags,
  EditValue,
  RenumberTracks,
  SetColumnValue,
  AutoCompleteTags,
  Rescan,
  FetchMetadata,
  Transcode,
  CopyUrl,
  ShowInCollection,
  OpenInFileManager,
  Organize,
  CopyToCollection,
  MoveToCollection,
  CopyToDevice,
  DeleteFiles,
  ClearPlaylist,
  Shuffle,
  RemoveDuplicates,
  RemoveUnavailable
};

struct Item {
  const char *id = "";
  Action action = Action::Play;
};

struct RowInfo {
  bool local_file = false;
  bool streaming = false;
  bool cue = false;
  bool editable = false;
  bool in_queue = false;
  bool skipped = false;
  bool pause_disabled = false;
};

struct SelectionState {
  int selected = 0;
  int local_songs = 0;
  int editable = 0;
  int streaming = 0;
  int in_queue = 0;
  int not_in_queue = 0;
  int in_skipped = 0;
  int not_in_skipped = 0;
  bool cue_selected = false;
  bool index_valid = false;
  bool track_column = false;
  bool column_editable = false;
  bool collection_item = false;
  bool playing_selected = false;
  bool pause_disabled = false;
  bool devices_connected = false;
  bool delete_allowed = false;
  PlaylistColumn column = PlaylistColumn::Title;
  std::string column_name;
  std::string column_value;
};

inline std::vector<Item> Items() {
  return {
      {"play", Action::Play},
      {"stop", Action::Stop},
      {"stop-after", Action::StopAfter},
      {"queue", Action::Queue},
      {"queue-next", Action::QueueNext},
      {"skip", Action::Skip},
      {"add-to-playlist", Action::AddToPlaylist},
      {"remove", Action::Remove},
      {"edit-tags", Action::EditTags},
      {"edit-value", Action::EditValue},
      {"renumber", Action::RenumberTracks},
      {"set-column", Action::SetColumnValue},
      {"autocomplete", Action::AutoCompleteTags},
      {"rescan", Action::Rescan},
      {"fetch", Action::FetchMetadata},
      {"transcode", Action::Transcode},
      {"copy-url", Action::CopyUrl},
      {"show-collection", Action::ShowInCollection},
      {"browse", Action::OpenInFileManager},
      {"organize", Action::Organize},
      {"copy-collection", Action::CopyToCollection},
      {"move-collection", Action::MoveToCollection},
      {"copy-device", Action::CopyToDevice},
      {"delete", Action::DeleteFiles},
      {"clear", Action::ClearPlaylist},
      {"shuffle", Action::Shuffle},
      {"remove-duplicates", Action::RemoveDuplicates},
      {"remove-unavailable", Action::RemoveUnavailable},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Play;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Play;
}

inline RowInfo FromSong(const Song &song, bool in_queue = false) {
  RowInfo row;
  row.local_file = song.is_local_file();
  row.streaming = song.is_stream();
  row.cue = !song.cue_path().empty();
  row.editable = song.IsEditable();
  row.in_queue = in_queue;
  row.skipped = song.skipped();
  row.pause_disabled = PlayerItemOptions::PauseDisabled(song);
  return row;
}

// Qt playlist context Play uses the item under the cursor (PauseDisabled on radio).
inline void ApplyContextSong(SelectionState *opts, const Song &song, bool playing) {
  if (!opts) {
    return;
  }
  opts->pause_disabled = PlayerItemOptions::PauseDisabled(song);
  opts->playing_selected = playing;
}

inline SelectionState Analyze(const std::vector<RowInfo> &rows, SelectionState opts = {}) {
  opts.selected = static_cast<int>(rows.size());
  opts.local_songs = 0;
  opts.editable = 0;
  opts.streaming = 0;
  opts.in_queue = 0;
  opts.not_in_queue = 0;
  opts.in_skipped = 0;
  opts.not_in_skipped = 0;
  opts.cue_selected = false;
  for (const RowInfo &row : rows) {
    if (row.local_file) {
      ++opts.local_songs;
    }
    if (row.streaming) {
      ++opts.streaming;
    }
    if (row.cue) {
      opts.cue_selected = true;
    } else if (row.editable) {
      ++opts.editable;
    }
    if (row.in_queue) {
      ++opts.in_queue;
    } else {
      ++opts.not_in_queue;
    }
    if (row.skipped) {
      ++opts.in_skipped;
    } else {
      ++opts.not_in_skipped;
    }
  }
  return opts;
}

inline bool LocalEditable(const SelectionState &state) { return state.local_songs > 0 && state.editable > 0; }

inline bool IncludeItem(Action action, const SelectionState &state) {
  switch (action) {
    case Action::Play:
    case Action::Stop:
    case Action::StopAfter:
    case Action::ClearPlaylist:
    case Action::Shuffle:
    case Action::RemoveDuplicates:
    case Action::RemoveUnavailable:
      return true;
    case Action::Queue:
    case Action::QueueNext:
    case Action::Skip:
    case Action::AddToPlaylist:
    case Action::Remove:
    case Action::CopyUrl:
      return state.selected > 0;
    case Action::EditTags:
    case Action::AutoCompleteTags:
    case Action::Rescan:
    case Action::Transcode:
      return LocalEditable(state);
    case Action::EditValue:
      return state.index_valid && state.editable > 0 && !state.cue_selected && state.column_editable;
    case Action::RenumberTracks:
      return state.local_songs > 0 && !state.cue_selected && state.editable >= 2 && state.track_column;
    case Action::SetColumnValue:
      return state.index_valid && state.editable >= 2 && !state.cue_selected && !state.track_column && state.column_editable;
    case Action::FetchMetadata:
      return state.streaming > 0;
    case Action::ShowInCollection:
      return state.index_valid && state.collection_item;
    case Action::OpenInFileManager:
      return state.selected > 0 && (state.local_songs == state.selected || (state.index_valid && state.collection_item));
    case Action::Organize:
      return state.index_valid && state.collection_item && LocalEditable(state) && !state.cue_selected;
    case Action::CopyToCollection:
    case Action::MoveToCollection:
      return state.index_valid && !state.collection_item && state.local_songs > 0;
    case Action::CopyToDevice:
      return state.index_valid && state.local_songs > 0;
    case Action::DeleteFiles:
      return state.index_valid && state.local_songs > 0 && state.delete_allowed;
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

inline bool PlayEnabled(const SelectionState &state) {
  if (!state.index_valid) {
    return false;
  }
  return !state.playing_selected || !state.pause_disabled;
}

// Qt indexAt / playlist_menu_index_: only a real row is a context target.
inline int ContextRow(int row_at_y, int row_count) {
  if (row_at_y < 0 || row_at_y >= row_count) {
    return -1;
  }
  return row_at_y;
}

// Qt PlaylistView::contextMenuEvent Keyboard (Menu / Shift+F10) uses currentIndex().
constexpr unsigned kMenuKey = 0xff67;
constexpr unsigned kF10Key = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;
constexpr double kKeyboardY = -1;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenuKey || (keyval == kF10Key && (state & kShiftMask) != 0);
}

inline bool IsKeyboardAnchor(double y) { return y < 0; }

inline int ContextRowFromKeyboard(int selected_row, int row_count) { return ContextRow(selected_row, row_count); }

inline bool StopAfterEnabled(const SelectionState &state) { return state.index_valid; }

inline bool CopyToDeviceEnabled(const SelectionState &state) { return state.devices_connected; }

inline bool SetColumnOpensDialog() { return false; }

inline std::string ClickedColumnValue(const std::string &value) { return value; }

// Qt SelectionSetValue reads playlist_menu_index_ (the right-clicked cell), not selection.front().
inline int SetColumnSourceRow(int menu_row, const std::vector<int> &selected) {
  if (menu_row >= 0) {
    return menu_row;
  }
  return selected.empty() ? -1 : selected.front();
}

inline std::string TruncatedValue(const std::string &value) {
  if (value.size() <= 25) {
    return value;
  }
  return value.substr(0, 25) + "...";
}

inline std::string LowerAscii(const std::string &value) {
  std::string out = value;
  for (char &c : out) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c - 'A' + 'a');
    }
  }
  return out;
}

inline std::string LabelFor(Action action, const SelectionState &state) {
  switch (action) {
    case Action::Play:
      return state.playing_selected ? "Pause" : "Play";
    case Action::Stop:
      return "Stop";
    case Action::StopAfter:
      return "Stop after this track";
    case Action::Queue:
      if (state.in_queue == 1 && state.not_in_queue == 0) {
        return "Dequeue track";
      }
      if (state.in_queue > 1 && state.not_in_queue == 0) {
        return "Dequeue selected tracks";
      }
      if (state.in_queue == 0 && state.not_in_queue == 1) {
        return "Queue track";
      }
      if (state.in_queue == 0 && state.not_in_queue > 1) {
        return "Queue selected tracks";
      }
      return "Toggle queue status";
    case Action::QueueNext:
      return state.selected > 1 ? "Queue selected tracks to play next" : "Queue to play next";
    case Action::Skip:
      if (state.in_skipped == 1 && state.not_in_skipped == 0) {
        return "Unskip track";
      }
      if (state.in_skipped > 1 && state.not_in_skipped == 0) {
        return "Unskip selected tracks";
      }
      if (state.in_skipped == 0 && state.not_in_skipped == 1) {
        return "Skip track";
      }
      if (state.in_skipped == 0 && state.not_in_skipped > 1) {
        return "Skip selected tracks";
      }
      return "Toggle skip status";
    case Action::AddToPlaylist:
      return "Add to another playlist";
    case Action::Remove:
      return "Remove from playlist";
    case Action::EditTags:
      return "Edit track information…";
    case Action::EditValue:
      return state.column_name.empty() ? "Edit value" : "Edit tag \"" + state.column_name + "\"...";
    case Action::RenumberTracks:
      return "Renumber tracks";
    case Action::SetColumnValue:
      return "Set " + LowerAscii(state.column_name) + " to \"" + TruncatedValue(state.column_value) + "\"...";
    case Action::AutoCompleteTags:
      return "Auto-complete tags…";
    case Action::Rescan:
      return "Rescan song(s)...";
    case Action::FetchMetadata:
      return "Fetch metadata from service";
    case Action::Transcode:
      return "Add files to transcoder";
    case Action::CopyUrl:
      return "Copy URL(s)...";
    case Action::ShowInCollection:
      return "Show in collection...";
    case Action::OpenInFileManager:
      return "Show in file browser...";
    case Action::Organize:
      return "Organize files...";
    case Action::CopyToCollection:
      return "Copy to collection...";
    case Action::MoveToCollection:
      return "Move to collection...";
    case Action::CopyToDevice:
      return "Copy to device...";
    case Action::DeleteFiles:
      return "Delete from disk...";
    case Action::ClearPlaylist:
      return "Clear playlist";
    case Action::Shuffle:
      return "Shuffle";
    case Action::RemoveDuplicates:
      return "Remove duplicates";
    case Action::RemoveUnavailable:
      return "Remove unavailable";
  }
  return {};
}

inline const char *WinAction(Action action) {
  switch (action) {
    case Action::Play:
      return "win.playlist-play";
    case Action::Stop:
      return "win.stop";
    case Action::StopAfter:
      return "win.playlist-stop-after";
    case Action::Queue:
      return "win.playlist-queue";
    case Action::QueueNext:
      return "win.queue-next";
    case Action::Skip:
      return "win.playlist-skip";
    case Action::AddToPlaylist:
      return "";
    case Action::Remove:
      return "win.playlist-remove";
    case Action::EditTags:
      return "win.edittag";
    case Action::EditValue:
      return "win.edit-value";
    case Action::RenumberTracks:
      return "win.renumber-tracks";
    case Action::SetColumnValue:
      return "win.set-column";
    case Action::AutoCompleteTags:
      return "win.autocomplete-tags";
    case Action::Rescan:
      return "win.rescan-selected";
    case Action::FetchMetadata:
      return "win.fetch-metadata";
    case Action::Transcode:
      return "win.transcode-selected";
    case Action::CopyUrl:
      return "win.copy-url";
    case Action::ShowInCollection:
      return "win.show-in-collection";
    case Action::OpenInFileManager:
      return "win.open-file-manager";
    case Action::Organize:
      return "win.organize-selected";
    case Action::CopyToCollection:
      return "win.copy-collection";
    case Action::MoveToCollection:
      return "win.move-collection";
    case Action::CopyToDevice:
      return "win.playlist-copy-device";
    case Action::DeleteFiles:
      return "win.delete-files";
    case Action::ClearPlaylist:
      return "win.clear-playlist";
    case Action::Shuffle:
      return "win.shuffle-playlist";
    case Action::RemoveDuplicates:
      return "win.remove-duplicates";
    case Action::RemoveUnavailable:
      return "win.remove-unavailable";
  }
  return "win.playlist-play";
}

}  // namespace PlaylistMenu

#endif
