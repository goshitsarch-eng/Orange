#ifndef STRAWBERRY_MAINWINDOWSEARCHFOCUS_H
#define STRAWBERRY_MAINWINDOWSEARCHFOCUS_H

#include <string>

namespace MainWindowSearchFocus {

enum class Target { None, Collection, Streaming, Playlist };

// Qt MainWindow::FocusSearchField: collection or the current streaming service first,
// then the playlist filter. Already-focused fields fall through to the next target.
inline Target Resolve(const std::string &sidebar_page, bool collection_focused, bool streaming_focused, bool playlist_focused) {
  if (sidebar_page == "collection" && !collection_focused) {
    return Target::Collection;
  }
  if (sidebar_page == "streaming" && !streaming_focused) {
    return Target::Streaming;
  }
  if (!playlist_focused) {
    return Target::Playlist;
  }
  return Target::None;
}

}  // namespace MainWindowSearchFocus

#endif
