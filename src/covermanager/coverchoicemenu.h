#ifndef STRAWBERRY_COVERCHOICEMENU_H
#define STRAWBERRY_COVERCHOICEMENU_H

#include <cstring>
#include <string>
#include <vector>

namespace CoverChoiceMenu {

enum class Action { Show, Search, File, Url, Save, Fetch, Unset, Clear, Delete };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::Show;
};

inline std::vector<Item> Items() {
  return {
      {"Show cover", "show", Action::Show},
      {"Search for cover…", "search", Action::Search},
      {"Load from file…", "file", Action::File},
      {"Load from URL…", "url", Action::Url},
      {"Save cover to file…", "save", Action::Save},
      {"Fetch cover", "fetch", Action::Fetch},
      {"Unset cover", "unset", Action::Unset},
      {"Clear cover", "clear", Action::Clear},
      {"Delete cover", "delete", Action::Delete},
  };
}

inline std::string ActionPath(const char *prefix, const char *id) {
  return std::string(prefix ? prefix : "cover") + "." + (id ? id : "");
}

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Show;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Show;
}

inline bool HasCoverActions(bool has_callback, bool song_valid) { return has_callback && song_valid; }

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline const char *SearchAutomaticallyLabel() { return "Fetch automatically"; }

inline const char *SearchAutomaticallyId() { return "auto"; }

inline std::string SearchAutomaticallyPath(const char *prefix) { return ActionPath(prefix, SearchAutomaticallyId()); }

inline bool IsSearchAutomatically(const char *id) { return id && std::strcmp(id, SearchAutomaticallyId()) == 0; }

inline int ItemCountWithAutoSearch() { return ItemCount() + 1; }

// Qt ContextAlbum::contextMenuEvent and AlbumCoverChoiceController attached
// menus pop from Menu / Shift+F10. GTK was pointer-only.
constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

// GTK AttachMenu shows when the song is valid or has a URL.
inline bool ShouldShowAttachedMenu(bool song_valid, bool has_url) { return song_valid || has_url; }

// Qt ContextAlbum::contextMenuEvent also requires a real cover (not the placeholder).
inline bool ShouldShowContextAlbumMenu(bool song_valid, bool has_url, bool has_cover) {
  return ShouldShowAttachedMenu(song_valid, has_url) && has_cover;
}

}  // namespace CoverChoiceMenu

#endif
