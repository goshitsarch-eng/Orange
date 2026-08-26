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

  RadioView();
  ~RadioView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Reload(RadioModel *model);
  void SetActivateCallback(std::function<void(const RadioChannel &)> callback) { activate_ = std::move(callback); }
  void SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }
  std::vector<RadioChannel> SelectedChannels() const;
  SongList SelectedSongs() const;

 private:
  void SetupRowDrag(GtkWidget *row, const RadioChannel &channel);
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const RadioChannel &)> activate_;
  MenuCallback menu_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
