#ifndef STRAWBERRY_TRACKSLIDERSLIDER_H
#define STRAWBERRY_TRACKSLIDERSLIDER_H

#include <functional>
#include <gtk/gtk.h>

class TrackSliderSlider {
 public:
  TrackSliderSlider();
  GtkWidget *widget() const { return widget_; }
  void SetValue(double value);
  double Value() const;
  void SetSeekCallback(std::function<void(double)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  std::function<void(double)> seek_;
};

#endif
