#ifndef STRAWBERRY_TRACKSLIDER_H
#define STRAWBERRY_TRACKSLIDER_H

#include "widgets/prettyslider.h"

#include <gtk/gtk.h>

#include <cstdint>
#include <functional>
#include <string>

class TrackSlider {
 public:
  using SeekCallback = std::function<void(int64_t)>;

  TrackSlider();

  GtkWidget *widget() const { return widget_; }
  PrettySlider *slider() { return &slider_; }

  void SetTimes(int64_t position_nanosec, int64_t length_nanosec);
  void SetSeekCallback(SeekCallback callback);
  void SetSliderVisible(bool visible);
  void ToggleRemaining();
  bool show_remaining() const { return show_remaining_; }
  const std::string &position_text() const { return position_text_; }
  const std::string &duration_text() const { return duration_text_; }

 private:
  void UpdateLabels();
  void PersistRemaining();

  PrettySlider slider_;
  GtkWidget *widget_ = nullptr;
  GtkWidget *position_label_ = nullptr;
  GtkWidget *duration_label_ = nullptr;
  int64_t position_nanosec_ = 0;
  int64_t length_nanosec_ = 0;
  bool show_remaining_ = false;
  SeekCallback seek_;
  std::string position_text_;
  std::string duration_text_;
};

#endif
