#ifndef STRAWBERRY_RADIOVIEW_H
#define STRAWBERRY_RADIOVIEW_H

#include "core/song.h"
#include "radios/radiomodel.h"

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class RadioView {
 public:
  using MenuCallback = std::function<void(const std::vector<RadioChannel> &)>;
  using RefreshCallback = std::function<void()>;
  using EnqueueCallback = std::function<void(const SongList &)>;

  RadioView();
  ~RadioView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Reload(RadioModel *model);
  void SetActivateCallback(std::function<void(const RadioChannel &)> callback) { activate_ = std::move(callback); }
  void SetEnqueueCallback(EnqueueCallback callback) { enqueue_ = std::move(callback); }
  void SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  void SetRefreshCallback(RefreshCallback callback) { refresh_ = std::move(callback); }
  std::vector<RadioChannel> SelectedChannels() const;
  SongList SelectedSongs() const;

 private:
  void SetupRowDrag(GtkWidget *row, const RadioChannel &channel);
  gboolean OnKeyPressed(guint keyval);
  bool ApplyTreeLeft();
  void SelectService(Song::Source source);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  RadioModel *model_ = nullptr;
  std::function<void(const RadioChannel &)> activate_;
  EnqueueCallback enqueue_;
  MenuCallback menu_;
  RefreshCallback refresh_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
