#ifndef STRAWBERRY_PLAYLISTTABBAR_H
#define STRAWBERRY_PLAYLISTTABBAR_H

#include "playlist/playlistlistlook.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlisttabmenu.h"
#include "widgets/favoritewidget.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistTabBar {
 public:
  using ChangedCallback = std::function<void(const std::string &)>;
  using FavoriteCallback = std::function<void(const std::string &, bool)>;
  using CloseCallback = std::function<void(int)>;
  using RenameCallback = std::function<void(int, const std::string &)>;
  using SaveCallback = std::function<void(int)>;
  using NewCallback = std::function<void()>;
  using LoadCallback = std::function<void()>;
  using ReorderCallback = std::function<void(const std::vector<int> &)>;
  using LastTabCloseCallback = std::function<void()>;
  using DropCallback = std::function<void(int, const std::string &)>;

  PlaylistTabBar();
  ~PlaylistTabBar();

  GtkWidget *widget() const { return widget_; }
  void Refresh(PlaylistManager *manager, const std::string &active_name = {},
               PlaylistListLook::Playback playback = PlaylistListLook::Playback::Stopped);
  void SetChangedCallback(ChangedCallback callback);
  void SetFavoriteCallback(FavoriteCallback callback);
  void SetCloseCallback(CloseCallback callback) { close_ = std::move(callback); }
  void SetRenameCallback(RenameCallback callback) { rename_ = std::move(callback); }
  void SetSaveCallback(SaveCallback callback) { save_ = std::move(callback); }
  void SetNewCallback(NewCallback callback) { new_ = std::move(callback); }
  void SetLoadCallback(LoadCallback callback) { load_ = std::move(callback); }
  void SetReorderCallback(ReorderCallback callback) { reorder_ = std::move(callback); }
  void SetLastTabCloseCallback(LastTabCloseCallback callback) { last_close_ = std::move(callback); }
  void SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);

 private:
  int TabCount() const;
  int CurrentIndex() const;
  int TabIdAt(int index) const;
  std::string TabNameAt(int index) const;
  std::vector<int> TabIds() const;
  int IndexOfWidget(GtkWidget *widget) const;
  int IndexAt(double x, double y) const;
  const char *PartAt(double x, double y) const;
  GtkWidget *TabWidget(int index) const;
  void ShowContextMenu(int index, double x, double y);
  void ActivateAction(PlaylistTabMenu::Action action, int index);
  void StartInlineRename(int index);
  void ApplyInlineRename();
  void HideEditor();
  void RequestClose(int id);
  void StartDragHover(int index);
  void CancelDragHover();
  void SetupTab(GtkWidget *tab, int index, int id, const std::string &name);
  void UpdateVisibility(bool show);
  void TickVisibility();
  void StopVisibilityAnim();

  GtkWidget *widget_ = nullptr;
  GtkWidget *popover_ = nullptr;
  GtkWidget *rename_entry_ = nullptr;
  GtkWidget *rename_button_ = nullptr;
  ChangedCallback changed_;
  FavoriteCallback favorite_;
  CloseCallback close_;
  RenameCallback rename_;
  SaveCallback save_;
  NewCallback new_;
  LoadCallback load_;
  ReorderCallback reorder_;
  LastTabCloseCallback last_close_;
  DropCallback drop_;
  std::vector<std::unique_ptr<FavoriteWidget>> favorites_;
  int menu_index_ = -1;
  int rename_id_ = -1;
  int hover_index_ = -1;
  guint hover_timeout_ = 0;
  bool shown_ = false;
  bool anim_showing_ = false;
  int anim_natural_ = 0;
  gint64 anim_start_us_ = 0;
  guint anim_id_ = 0;
};

#endif
