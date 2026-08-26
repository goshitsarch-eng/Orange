#ifndef STRAWBERRY_PLAYINGWIDGET_H
#define STRAWBERRY_PLAYINGWIDGET_H

#include "core/song.h"

#include <gtk/gtk.h>

#include <vector>

class PlayingWidget {
 public:
  PlayingWidget();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *cover() const { return cover_; }
  bool IsEnabled() const { return enabled_; }
  void SetEnabled(bool enabled);
  void SongChanged(const Song &song);
  void SetCover(const std::vector<unsigned char> &data);
  const Song &song() const { return song_; }

 private:
  void SetImageFromBytes(const std::vector<unsigned char> &data);

  GtkWidget *widget_ = nullptr;
  GtkWidget *cover_ = nullptr;
  GtkWidget *title_ = nullptr;
  GtkWidget *artist_ = nullptr;
  bool enabled_ = true;
  Song song_;
};

#endif
