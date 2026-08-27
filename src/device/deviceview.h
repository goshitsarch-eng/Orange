#ifndef STRAWBERRY_DEVICEVIEW_H
#define STRAWBERRY_DEVICEVIEW_H

#include "collection/collectionitem.h"
#include "collection/collectionmodel.h"
#include "core/song.h"
#include "device/connecteddevice.h"

#include <functional>
#include <set>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class DeviceView {
 public:
  DeviceView();
  ~DeviceView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void ShowDevices(const std::vector<ConnectedDevice> &devices);
  void ShowSongs(const SongList &songs);
  void SetDeviceCallback(std::function<void(const std::string &)> callback) { device_cb_ = std::move(callback); }
  void SetSongCallback(std::function<void(const Song &)> callback) { song_cb_ = std::move(callback); }
  void SetActivateSongsCallback(std::function<void(const SongList &)> callback) { songs_cb_ = std::move(callback); }
  void SetEnqueueCallback(std::function<void(const SongList &)> callback) { enqueue_ = std::move(callback); }
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  void SetBackCallback(std::function<void()> callback) { back_cb_ = std::move(callback); }
  void SetAddAllCallback(std::function<void()> callback) { add_all_cb_ = std::move(callback); }
  void SetDeviceMenuCallback(std::function<void(const ConnectedDevice &)> callback) { device_menu_cb_ = std::move(callback); }
  void SetSongMenuCallback(std::function<void(const Song &)> callback) { song_menu_cb_ = std::move(callback); }

  const ConnectedDevice *SelectedDevice() const;
  SongList SelectedSongs() const;

 private:
  void Clear();
  void RebuildSongs();
  void AppendItem(const CollectionItem *item, int depth);
  void ToggleExpanded(const CollectionItem *item);
  void AttachMenu(GtkWidget *row);
  void SetupRowDrag(GtkWidget *row, const Song &song);
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  CollectionModel model_;
  std::set<std::string> expanded_;
  SongList songs_;
  std::function<void(const std::string &)> device_cb_;
  std::function<void(const Song &)> song_cb_;
  std::function<void(const SongList &)> songs_cb_;
  std::function<void(const SongList &)> enqueue_;
  std::function<void()> back_cb_;
  std::function<void()> add_all_cb_;
  std::function<void(const ConnectedDevice &)> device_menu_cb_;
  std::function<void(const Song &)> song_menu_cb_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
