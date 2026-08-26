#ifndef STRAWBERRY_PLAYLISTCONTAINER_H
#define STRAWBERRY_PLAYLISTCONTAINER_H

#include "playlist/playlisttabbar.h"
#include "playlist/playlistview.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

class PlaylistManager;

class PlaylistContainer {
 public:
  using ActionCallback = std::function<void()>;

  PlaylistContainer();

  GtkWidget *widget() const { return widget_; }
  PlaylistView *view() { return view_.get(); }
  PlaylistTabBar *tab_bar() { return tab_bar_.get(); }
  GtkWidget *summary() const { return summary_; }
  GtkWidget *repeat_button() const { return repeat_button_; }
  GtkWidget *shuffle_button() const { return shuffle_button_; }
  const std::string &filter_string() const { return filter_; }

  void SetFilterChangedCallback(const std::function<void(const std::string &)> &callback);
  void SetActionCallback(const char *name, ActionCallback callback);
  void SetSummary(const std::string &text);

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *summary_ = nullptr;
  GtkWidget *repeat_button_ = nullptr;
  GtkWidget *shuffle_button_ = nullptr;
  std::unique_ptr<PlaylistTabBar> tab_bar_;
  std::unique_ptr<PlaylistView> view_;
  std::string filter_;
  std::function<void(const std::string &)> filter_changed_;
};

#endif
