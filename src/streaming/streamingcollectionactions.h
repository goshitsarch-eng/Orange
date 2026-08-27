#ifndef STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H
#define STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H

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

inline bool FavoriteEnabled(int songs_selected, MenuContext ctx) {
  return ctx == MenuContext::Collection && songs_selected > 0;
}

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
