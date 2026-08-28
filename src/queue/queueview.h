#ifndef STRAWBERRY_QUEUEVIEW_H
#define STRAWBERRY_QUEUEVIEW_H

#include "core/song.h"
#include "playlist/playlistdropindicator.h"

#include <functional>
#include <string>
#include <vector>
#include <gtk/gtk.h>

class Queue;

class QueueView {
 public:
  using UrlDropCallback = std::function<void(const std::vector<std::string> &urls, int dest)>;
  using PlaylistRowsDropCallback = std::function<void(const std::vector<int> &rows, int dest)>;

  explicit QueueView(Queue *queue);
  ~QueueView();
  GtkWidget *widget() const { return widget_; }
  void SetQueue(Queue *queue);
  Queue *queue() const { return queue_; }
  void Reload();
  void ApplyLook();
  void MoveUp();
  void MoveDown();
  void Remove();
  void Clear();
  void SetActivateCallback(std::function<void(const Song &)> callback);
  void SetUrlDropCallback(UrlDropCallback callback) { url_drop_ = std::move(callback); }
  void SetPlaylistRowsDropCallback(PlaylistRowsDropCallback callback) { playlist_drop_ = std::move(callback); }
  void SetNowPlayingUrl(const std::string &url);
  const std::string &now_playing_url() const { return now_playing_url_; }
  std::vector<int> SelectedIndexes() const;

  static bool IsNowPlaying(const Song &song, const std::string &url) { return !url.empty() && song.url() == url; }

 private:
  void Rebuild();
  void SetupRowDrag(GtkWidget *row, int index);
  int RowAtY(double y) const;
  void UpdateDropIndicator(double y);
  void ClearDropIndicator();
  gboolean OnDrop(const GValue *value, double y);
  gboolean OnKeyPressed(guint keyval, GdkModifierType modifiers);
  void ResetTypeAhead();
  void UpdateChrome();

  Queue *queue_ = nullptr;
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  GtkWidget *drop_overlay_ = nullptr;
  PlaylistDropIndicator::State drop_state_;
  GtkWidget *summary_ = nullptr;
  GtkWidget *move_up_ = nullptr;
  GtkWidget *move_down_ = nullptr;
  GtkWidget *remove_ = nullptr;
  GtkWidget *clear_ = nullptr;
  std::function<void(Song)> activate_;
  UrlDropCallback url_drop_;
  PlaylistRowsDropCallback playlist_drop_;
  std::string now_playing_url_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
