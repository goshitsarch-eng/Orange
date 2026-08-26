#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEW_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEW_H

#include "core/song.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class StreamingCollectionView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;
  using RefreshCallback = std::function<void()>;
  using MenuCallback = std::function<void(const SongList &)>;

  explicit StreamingCollectionView(const std::string &title);
  ~StreamingCollectionView();

  GtkWidget *widget() const { return widget_; }
  void SetSongs(const SongList &songs);
  void PushSongs(const SongList &songs);
  void PopBrowse();
  bool CanGoBack() const { return !stack_.empty(); }
  void SetStatus(const std::string &status);
  void SetFilter(const std::string &filter);
  void SetActivateCallback(ActivateCallback callback);
  void SetRefreshCallback(RefreshCallback callback);
  void SetMenuCallback(MenuCallback callback);
  const SongList &songs() const { return songs_; }
  SongList Visible() const;
  SongList SelectedSongs() const;

 private:
  struct Level {
    SongList songs;
    std::string status;
  };

  void Rebuild();
  void SetupRowDrag(GtkWidget *row, const Song &song);
  void UpdateBack();
  void ActivateSong(const Song &song);

  GtkWidget *widget_ = nullptr;
  GtkWidget *back_ = nullptr;
  GtkWidget *filter_entry_ = nullptr;
  GtkWidget *status_label_ = nullptr;
  GtkWidget *list_ = nullptr;
  SongList songs_;
  std::vector<Level> stack_;
  std::string filter_;
  ActivateCallback activate_;
  RefreshCallback refresh_;
  MenuCallback menu_;
};

#endif
