#ifndef STRAWBERRY_PLAYLISTTABMENU_H
#define STRAWBERRY_PLAYLISTTABMENU_H

#include "playlist/playlistlistdrop.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace PlaylistTabMenu {

enum class Action { Star, Close, Rename, Save, New, Load };

enum class Click { None, Close, Rename, New };

struct Item {
  const char *id = "";
  const char *label = "";
  Action action = Action::Star;
  bool separator_before = false;
};

inline constexpr const char *kTabPrefix = "strawberry-playlist-tab:";
inline constexpr int kDragHoverTimeoutMs = 500;

inline std::vector<Item> Items() {
  return {{"star", "Star playlist", Action::Star, false},
          {"close", "Close playlist", Action::Close, false},
          {"rename", "Rename playlist...", Action::Rename, false},
          {"save", "Save playlist...", Action::Save, false},
          {"new", "New playlist", Action::New, true},
          {"load", "Load playlist...", Action::Load, true}};
}

inline bool ActionEnabled(Action action, int index, int count) {
  switch (action) {
    case Action::Star:
    case Action::Close:
      return index >= 0 && count > 1;
    case Action::Rename:
    case Action::Save:
      return index >= 0;
    case Action::New:
    case Action::Load:
      return true;
  }
  return false;
}

inline Click FromRelease(int index, unsigned button) {
  if (button == 2 && index >= 0) {
    return Click::Close;
  }
  return Click::None;
}

inline Click FromPress(int index, unsigned button, int n_press) {
  if (button == 2 || n_press != 2) {
    return Click::None;
  }
  return index < 0 ? Click::New : Click::Rename;
}

inline bool ShouldApplyRename(const std::string &old_name, const std::string &new_name) {
  return !new_name.empty() && new_name != old_name;
}

inline bool CloseCurrentHidesWindow(int count) { return count <= 1; }

inline bool ToggledFavorite(bool favorite) { return !favorite; }

inline const char *FavoriteTooltip() {
  return "Double-click here to favorite this playlist so it will be saved and remain accessible through the \"Playlists\" panel on the left side bar";
}

inline std::string TabPayload(int id) { return std::string(kTabPrefix) + std::to_string(id); }

inline bool IsTabPayload(const std::string &text) { return text.rfind(kTabPrefix, 0) == 0; }

inline int ParseTabId(const std::string &text) {
  if (!IsTabPayload(text)) {
    return -1;
  }
  return std::atoi(text.c_str() + std::strlen(kTabPrefix));
}

inline bool ShouldHoverForPayload(const std::string &payload) {
  if (payload.empty() || IsTabPayload(payload)) {
    return false;
  }
  return PlaylistListDrop::IsPlaylistRows(payload) || !PlaylistListDrop::ParseUrls(payload).empty();
}

inline bool DropOnEmptyCreatesPlaylist() { return true; }

inline std::vector<int> ReorderIds(std::vector<int> ids, int from_index, int dest_index) {
  if (from_index < 0 || dest_index < 0 || from_index >= static_cast<int>(ids.size())) {
    return ids;
  }
  const int id = ids[from_index];
  ids.erase(ids.begin() + static_cast<std::vector<int>::difference_type>(from_index));
  int dest = dest_index;
  if (dest > from_index) {
    --dest;
  }
  if (dest < 0) {
    dest = 0;
  }
  if (dest > static_cast<int>(ids.size())) {
    dest = static_cast<int>(ids.size());
  }
  ids.insert(ids.begin() + static_cast<std::vector<int>::difference_type>(dest), id);
  return ids;
}

}  // namespace PlaylistTabMenu

#endif
