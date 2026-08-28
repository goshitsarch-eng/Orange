#ifndef STRAWBERRY_SMARTPLAYLISTSEARCHPREVIEW_H
#define STRAWBERRY_SMARTPLAYLISTSEARCHPREVIEW_H

#include "core/song.h"
#include "smartplaylists/smartplaylist.h"

#include <functional>
#include <memory>

#include <gtk/gtk.h>

class SmartPlaylistSearchPreview {
 public:
  SmartPlaylistSearchPreview();
  ~SmartPlaylistSearchPreview();

  using FinishedCallback = std::function<void(int)>;

  GtkWidget *widget() const { return widget_; }
  void Update(const SmartPlaylistSearch &search, const SongList &songs);
  void SetFinishedCallback(FinishedCallback callback) { finished_ = std::move(callback); }
  int match_count() const { return match_count_; }

  void OnSearchFinished(guint generation, const SmartPlaylistSearch &search, SongList matches);
  void OnMapped();

 private:
  bool Hidden() const;
  void RunSearch(const SmartPlaylistSearch &search);
  void ApplyResults(const SongList &matches);

  GtkWidget *widget_ = nullptr;
  GtkWidget *label_ = nullptr;
  GtkWidget *header_ = nullptr;
  GtkWidget *list_ = nullptr;
  int match_count_ = 0;
  SmartPlaylistSearch last_search_;
  bool have_last_search_ = false;
  SmartPlaylistSearch pending_;
  bool have_pending_ = false;
  bool busy_ = false;
  guint generation_ = 0;
  SongList library_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  FinishedCallback finished_;
};

#endif
