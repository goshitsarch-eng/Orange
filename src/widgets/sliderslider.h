#ifndef STRAWBERRY_SLIDERSLIDER_H
#define STRAWBERRY_SLIDERSLIDER_H

#include <gtk/gtk.h>

#include <functional>

class SliderSlider {
 public:
  using ChangedCallback = std::function<void(double)>;

  explicit SliderSlider(double min = 0, double max = 100, double step = 1);

  GtkWidget *widget() const { return widget_; }
  double value() const;
  void set_value(double value);
  void SetRange(double min, double max);
  void SetChangedCallback(ChangedCallback callback);
  void BlockSignals(bool block);

 protected:
  GtkWidget *widget_ = nullptr;
  ChangedCallback changed_;
  bool blocked_ = false;
};

#endif
