#ifndef STRAWBERRY_WINDOWS7THUMBBAR_H
#define STRAWBERRY_WINDOWS7THUMBBAR_H

#include "core/windows7thumbbaractions.h"

#include <functional>
#include <vector>

#ifdef _WIN32
#include <gtk/gtk.h>
#endif

class Windows7ThumbBar {
 public:
  using Activated = std::function<void(Windows7ThumbBarActions::Id)>;

#ifdef _WIN32
  explicit Windows7ThumbBar(GtkWidget *window);
  ~Windows7ThumbBar();
  void SetActions(const std::vector<Windows7ThumbBarActions::Id> &actions);
  void SetPlaying(bool playing);
  void HandleWinEvent(void *msg);
  void set_activated(Activated cb) { activated_ = std::move(cb); }

 private:
  void Rebuild();
  GtkWidget *window_ = nullptr;
  std::vector<Windows7ThumbBarActions::Id> actions_;
  bool playing_ = false;
  unsigned button_created_message_id_ = 0;
  void *taskbar_list_ = nullptr;
  Activated activated_;
#else
  Windows7ThumbBar() = default;
#endif
};

#endif
