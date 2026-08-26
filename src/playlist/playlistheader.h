#ifndef STRAWBERRY_PLAYLISTHEADER_H
#define STRAWBERRY_PLAYLISTHEADER_H

#include "playlist/playlistdelegates.h"

#include <gtk/gtk.h>

#include <functional>

class PlaylistHeader {
 public:
  using SortCallback = std::function<void(PlaylistColumn)>;

  PlaylistHeader();

  GtkWidget *widget() const { return widget_; }
  void Rebuild(const SortCallback &callback);

 private:
  GtkWidget *widget_ = nullptr;
};

#endif
