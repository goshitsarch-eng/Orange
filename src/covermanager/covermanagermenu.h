#ifndef STRAWBERRY_COVERMANAGERMENU_H
#define STRAWBERRY_COVERMANAGERMENU_H

#include "core/song.h"
#include "covermanager/coverchoicemenu.h"

#include <cstring>
#include <vector>

namespace CoverManagerMenu {

struct Extra {
  const char *label = "";
  const char *id = "";
};

struct CoverState {
  int selected = 0;
  bool some_with_covers = false;
  bool some_unset = false;
  bool some_clear = false;
  bool has_providers = true;
};

inline std::vector<CoverChoiceMenu::Item> CoverItems() { return CoverChoiceMenu::Items(); }

inline std::vector<Extra> PlaylistItems() {
  return {
      {"Add to playlist", "append"},
      {"Load to playlist", "load"},
  };
}

inline int CoverItemCount() { return CoverChoiceMenu::ItemCount(); }

inline int PlaylistItemCount() { return static_cast<int>(PlaylistItems().size()); }

inline int ItemCount() { return CoverItemCount() + PlaylistItemCount(); }

inline bool HasCover(const Song &song) {
  return song.art_embedded() || !song.art_automatic().empty() || !song.art_manual().empty();
}

inline bool IsClear(const Song &song) {
  return !song.art_unset() && !song.art_embedded() && song.art_automatic().empty() && song.art_manual().empty();
}

inline bool HasAnyProviders(size_t provider_count) { return provider_count > 0; }

inline CoverState Analyze(const std::vector<Song> &songs, bool has_providers = true, bool has_cover_override = false) {
  CoverState state;
  state.selected = static_cast<int>(songs.size());
  state.has_providers = has_providers;
  for (const Song &song : songs) {
    if (HasCover(song) || has_cover_override) {
      state.some_with_covers = true;
    }
    if (song.art_unset()) {
      state.some_unset = true;
    } else if (IsClear(song)) {
      state.some_clear = true;
    }
  }
  return state;
}

inline CoverState FromSong(const Song &song, bool has_providers = true, bool has_cover_override = false) {
  return Analyze({song}, has_providers, has_cover_override);
}

inline bool IncludeCoverItem(CoverChoiceMenu::Action action, const CoverState &state) {
  if (state.selected <= 0) {
    return false;
  }
  switch (action) {
    case CoverChoiceMenu::Action::Show:
      return state.some_with_covers && state.selected == 1;
    case CoverChoiceMenu::Action::Save:
    case CoverChoiceMenu::Action::Delete:
      return state.some_with_covers;
    case CoverChoiceMenu::Action::File:
    case CoverChoiceMenu::Action::Url:
      return state.selected == 1;
    case CoverChoiceMenu::Action::Search:
      return state.has_providers;
    case CoverChoiceMenu::Action::Fetch:
      return true;
    case CoverChoiceMenu::Action::Unset:
      return state.some_with_covers || state.some_clear;
    case CoverChoiceMenu::Action::Clear:
      return state.some_with_covers || state.some_unset;
  }
  return false;
}

inline std::vector<CoverChoiceMenu::Item> VisibleCoverItems(const CoverState &state) {
  std::vector<CoverChoiceMenu::Item> visible;
  for (const CoverChoiceMenu::Item &item : CoverItems()) {
    if (IncludeCoverItem(item.action, state)) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline bool Contains(const std::vector<CoverChoiceMenu::Item> &items, CoverChoiceMenu::Action action) {
  for (const CoverChoiceMenu::Item &item : items) {
    if (item.action == action) {
      return true;
    }
  }
  return false;
}

inline bool IncludePlaylistItems(const CoverState &state) { return state.selected > 0; }

inline bool HasSearch() {
  for (const CoverChoiceMenu::Item &item : CoverItems()) {
    if (item.action == CoverChoiceMenu::Action::Search) {
      return true;
    }
  }
  return false;
}

inline bool IsCoverId(const char *id) {
  if (!id) {
    return false;
  }
  for (const CoverChoiceMenu::Item &item : CoverItems()) {
    if (std::strcmp(item.id, id) == 0) {
      return true;
    }
  }
  return false;
}

inline bool IsPlaylistId(const char *id) {
  if (!id) {
    return false;
  }
  for (const Extra &item : PlaylistItems()) {
    if (std::strcmp(item.id, id) == 0) {
      return true;
    }
  }
  return false;
}

inline bool LoadReplacesPlaylist(const char *id) { return id && std::strcmp(id, "load") == 0; }

}  // namespace CoverManagerMenu

#endif
