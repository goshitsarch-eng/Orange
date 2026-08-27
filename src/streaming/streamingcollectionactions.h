#ifndef STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H
#define STRAWBERRY_STREAMINGCOLLECTIONACTIONS_H

#include <vector>

namespace StreamingCollectionActions {

// Qt StreamingCollectionView::contextMenuEvent and StreamingSearchView::ResultsContextMenuEvent.
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

inline bool RemoveFromFavoritesEnabled(int songs_selected) { return songs_selected > 0; }

// Qt StreamingCollectionView does not call setEnabled on Queue to play next.
inline bool EnqueueNextEnabled(int) { return true; }

inline bool SearchContextActionsEnabled(int songs_selected) { return songs_selected > 0; }

inline bool ActionEnabled(Action action, int songs_selected) {
  switch (action) {
    case Action::EnqueueNext:
      return EnqueueNextEnabled(songs_selected);
    case Action::Append:
      return AppendEnabled(songs_selected);
    case Action::Replace:
      return LoadEnabled(songs_selected);
    case Action::New:
      return OpenInNewEnabled(songs_selected);
    case Action::Enqueue:
      return EnqueueEnabled(songs_selected);
    case Action::Favorite:
      return SearchContextActionsEnabled(songs_selected);
    case Action::Unfavorite:
      return RemoveFromFavoritesEnabled(songs_selected);
  }
  return false;
}

inline std::vector<Item> VisibleItems(int songs_selected) {
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (ActionEnabled(item.id, songs_selected)) {
      visible.push_back(item);
    }
  }
  return visible;
}

}  // namespace StreamingCollectionActions

#endif
