#ifndef STRAWBERRY_QUEUEVIEW_H
#define STRAWBERRY_QUEUEVIEW_H

#include "core/song.h"

#include <functional>
#include <gtk/gtk.h>

class Queue;

class QueueView {
 public:
  explicit QueueView(Queue *queue);
  GtkWidget *widget() const { return widget_; }
  void Reload();
  void MoveUp();
  void MoveDown();
  void Remove();
  void Clear();
  void SetActivateCallback(std::function<void(const Song &)> callback);

 private:
  void Rebuild();
  int SelectedIndex() const;

  Queue *queue_ = nullptr;
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(Song)> activate_;
};

#endif
