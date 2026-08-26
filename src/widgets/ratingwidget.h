#ifndef STRAWBERRY_RATINGWIDGET_H
#define STRAWBERRY_RATINGWIDGET_H

#include "widgets/ratingpainter.h"

#include <gtk/gtk.h>

#include <functional>

class RatingWidget {
 public:
  using ChangedCallback = std::function<void(float)>;

  RatingWidget();

  GtkWidget *widget() const { return widget_; }
  float rating() const { return rating_; }
  void set_rating(float rating);
  void SetChangedCallback(ChangedCallback callback);

 private:
  void Refresh();

  GtkWidget *widget_ = nullptr;
  GtkWidget *label_ = nullptr;
  float rating_ = -1.0f;
  ChangedCallback changed_;
};

#endif
