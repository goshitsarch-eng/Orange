#ifndef STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H
#define STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H

#include <cstring>
#include <vector>

namespace StreamingCollectionActions {

// Qt StreamingCollectionView::contextMenuEvent vs StreamingSearchView::ResultsContextMenuEvent.
enum class MenuContext { Search, Collection };

enum class Action { Append, Replace, New, Enqueue, EnqueueNext, Favorite, Unfavorite };

struct Item {
  const char *label = "";
  const char *action = "";
  Action id = Action::Append;
};

inline std::vector<Item> Items() {
  return {
      {"Append to current playlist", "win.streaming-append", Action::Append},
      {"Replace current playlist", "win.streaming-replace", Action::Replace},
      {"Open in new playlist", "win.streaming-new", Action::New},
      {"Queue track", "win.streaming-enqueue", Action::Enqueue},
      {"Queue to play next", "win.streaming-enqueue-next", Action::EnqueueNext},
      {"Add to favorites", "win.streaming-favorite", Action::Favorite},
      {"Remove from favorites", "win.streaming-unfavorite", Action::Unfavorite},
  };
}

inline bool ShouldShowContextMenu(bool valid_index) { return valid_index; }

// Qt StreamingCollectionView / StreamingSearchView contextMenuEvent from Menu / Shift+F10.
constexpr unsigned kMenuKey = 0xff67;
constexpr unsigned kF10Key = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenuKey || (keyval == kF10Key && (state & kShiftMask) != 0);
}

inline bool SelectionActionsEnabled(int songs_selected) { return songs_selected > 0; }

inline bool LoadEnabled(int songs_selected) { return songs_selected > 0; }

inline bool AppendEnabled(int songs_selected) { return songs_selected > 0; }

inline bool OpenInNewEnabled(int songs_selected) { return songs_selected > 0; }

inline bool EnqueueEnabled(int songs_selected) { return songs_selected > 0; }

inline bool RemoveFromFavoritesEnabled(int songs_selected, MenuContext ctx) {
  return ctx == MenuContext::Collection && songs_selected > 0;
}

inline bool RemoveFromFavoritesEnabled(int songs_selected) {
  return RemoveFromFavoritesEnabled(songs_selected, MenuContext::Collection);
}

// Qt StreamingCollectionView adds Queue to play next and never calls setEnabled on it.
// Qt StreamingSearchView does not add that action.
inline bool EnqueueNextEnabled(int, MenuContext ctx) { return ctx == MenuContext::Collection; }

inline bool EnqueueNextEnabled(int songs_selected) { return EnqueueNextEnabled(songs_selected, MenuContext::Collection); }

// Qt search context_actions_ (add-to-collection) enable when the results have a selection.
inline bool SearchContextActionsEnabled(int songs_selected, MenuContext ctx) {
  return ctx == MenuContext::Search && songs_selected > 0;
}

inline bool SearchContextActionsEnabled(int songs_selected) {
  return SearchContextActionsEnabled(songs_selected, MenuContext::Search);
}

// Qt StreamingSearchView::ResultsContextMenuEvent: Search for this only when one row is selected.
inline bool SearchForThisEnabled(int songs_selected, MenuContext ctx) {
  return ctx == MenuContext::Search && songs_selected == 1;
}

inline bool SearchForThisEnabled(int songs_selected) { return SearchForThisEnabled(songs_selected, MenuContext::Search); }

// Qt StreamingCollectionView never adds "Add to favorites"; collection tabs only have Remove.
inline bool FavoriteEnabled(int, MenuContext) { return false; }

inline bool FavoriteEnabled(int songs_selected) { return FavoriteEnabled(songs_selected, MenuContext::Collection); }

inline const char *DisplayOptionsLabel() { return "Display options"; }

inline bool DisplayOptionsEnabled(MenuContext ctx) { return ctx == MenuContext::Collection; }

inline bool DisplayOptionsEnabled(int, MenuContext ctx) { return DisplayOptionsEnabled(ctx); }

inline bool HasDisplayOptionsTab(const char *tab) {
  return tab && (std::strcmp(tab, "artists") == 0 || std::strcmp(tab, "albums") == 0 || std::strcmp(tab, "songs") == 0 ||
                 std::strcmp(tab, "favorites") == 0);
}

inline const char *SearchGroupByLabel() { return "Group by"; }

// Qt StreamingSearchView::ResultsContextMenuEvent always adds Group by and Configure.
inline bool SearchSettingsEnabled(MenuContext ctx) { return ctx == MenuContext::Search; }

inline bool SearchSettingsEnabled(int, MenuContext ctx) { return SearchSettingsEnabled(ctx); }

inline bool ActionEnabled(Action action, int songs_selected, MenuContext ctx) {
  switch (action) {
    case Action::EnqueueNext:
      return EnqueueNextEnabled(songs_selected, ctx);
    case Action::Append:
      return AppendEnabled(songs_selected);
    case Action::Replace:
      return LoadEnabled(songs_selected);
    case Action::New:
      return OpenInNewEnabled(songs_selected);
    case Action::Enqueue:
      return EnqueueEnabled(songs_selected);
    case Action::Favorite:
      return FavoriteEnabled(songs_selected, ctx);
    case Action::Unfavorite:
      return RemoveFromFavoritesEnabled(songs_selected, ctx);
  }
  return false;
}

inline bool ActionEnabled(Action action, int songs_selected) {
  return ActionEnabled(action, songs_selected, MenuContext::Collection);
}

inline bool Contains(const std::vector<Item> &items, Action action) {
  for (const Item &item : items) {
    if (item.id == action) {
      return true;
    }
  }
  return false;
}

inline std::vector<Item> VisibleItems(int songs_selected, MenuContext ctx) {
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (ActionEnabled(item.id, songs_selected, ctx)) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline std::vector<Item> VisibleItems(int songs_selected) { return VisibleItems(songs_selected, MenuContext::Collection); }

}  // namespace StreamingCollectionActions

#endif
