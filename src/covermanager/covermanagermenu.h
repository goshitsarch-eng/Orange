#ifndef STRAWBERRY_COVERMANAGERMENU_H
#define STRAWBERRY_COVERMANAGERMENU_H

#include "covermanager/coverchoicemenu.h"

#include <cstring>
#include <vector>

namespace CoverManagerMenu {

struct Extra {
  const char *label = "";
  const char *id = "";
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
