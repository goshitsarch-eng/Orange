#ifndef STRAWBERRY_SMARTPLAYLISTSVIEW_H
#define STRAWBERRY_SMARTPLAYLISTSVIEW_H

#include "smartplaylists/smartplaylistsmodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

enum class SmartPlaylistsAction { Activate, Append, Replace, OpenInNew, Queue, QueueNext, Edit, Delete, New, RestoreDefaults };

class SmartPlaylistsView {
 public:
  SmartPlaylistsView();
  ~SmartPlaylistsView();

  using SongsCallback = std::function<SongList(const SmartPlaylistsItem &)>;

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Reload(SmartPlaylistsModel *model);
  void SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback) { activate_ = std::move(callback); }
  void SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback) { delete_ = std::move(callback); }
  void SetActionCallback(std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> callback) {
    action_ = std::move(callback);
  }
  void SetSongsCallback(SongsCallback callback) { songs_ = std::move(callback); }
  const SmartPlaylistsItem *SelectedItem() const;
  void Trigger(SmartPlaylistsAction action);

 private:
  void Emit(const SmartPlaylistsItem &item, SmartPlaylistsAction action);
  void ShowMenu(GtkWidget *relative, const SmartPlaylistsItem *item);
  void SetupRowDrag(GtkWidget *row, const SmartPlaylistsItem &item);
  gboolean OnKeyPressed(guint keyval);
  void ResetTypeAhead();

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const SmartPlaylistsItem &)> activate_;
  std::function<void(const SmartPlaylistsItem &)> delete_;
  std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> action_;
  SongsCallback songs_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
