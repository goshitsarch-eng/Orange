#ifndef STRAWBERRY_CONTEXTOPTIONS_H
#define STRAWBERRY_CONTEXTOPTIONS_H

#include <cstring>
#include <vector>

namespace ContextOptions {

enum class Action { ShowAlbum, ShowData, ShowLyrics, SearchLyrics };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::ShowAlbum;
  bool checkable = true;
};

inline std::vector<Item> Items() {
  return {
      {"Show album cover", "album", Action::ShowAlbum, true},
      {"Show song technical data", "data", Action::ShowData, true},
      {"Show song lyrics", "lyrics", Action::ShowLyrics, true},
      {"Automatically search for song lyrics", "search-lyrics", Action::SearchLyrics, true},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline Action FromId(const char *id) {
  if (!id) {
    return Action::ShowAlbum;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::ShowAlbum;
}

inline bool IsIdle(bool song_valid, bool has_url) { return !song_valid && !has_url; }

inline bool ShowIdleMenu(bool idle) { return idle; }

inline bool ShowCoverMenu(bool idle, bool has_cover) { return !idle && has_cover; }

inline bool TriggersLyricsSearch(Action action) { return action == Action::ShowLyrics || action == Action::SearchLyrics; }

inline bool Checked(Action action, bool show_album, bool show_data, bool show_lyrics, bool search_lyrics) {
  switch (action) {
    case Action::ShowAlbum:
      return show_album;
    case Action::ShowData:
      return show_data;
    case Action::ShowLyrics:
      return show_lyrics;
    case Action::SearchLyrics:
      return search_lyrics;
  }
  return false;
}

inline bool Toggle(Action action, bool show_album, bool show_data, bool show_lyrics, bool search_lyrics) {
  return !Checked(action, show_album, show_data, show_lyrics, search_lyrics);
}

}  // namespace ContextOptions

#endif
