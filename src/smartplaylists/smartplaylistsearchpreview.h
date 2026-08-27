#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHPREVIEW_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHPREVIEW_H

#include "core/song.h"
#include "smartplaylists/smartplaylist.h"

#include <gtk/gtk.h>

class SmartPlaylistSearchPreview {
 public:
  SmartPlaylistSearchPreview();

  GtkWidget *widget() const { return widget_; }
  void Update(const SmartPlaylistSearch &search, const SongList &songs);
  int match_count() const { return match_count_; }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *label_ = nullptr;
  GtkWidget *list_ = nullptr;
  int match_count_ = 0;
  SmartPlaylistSearch last_search_;
  bool have_last_search_ = false;
};

#endif
