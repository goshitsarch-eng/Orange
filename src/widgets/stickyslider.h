#ifndef STRAWBERRY_STICKYSLIDER_H
#define STRAWBERRY_STICKYSLIDER_H

#include "widgets/sliderslider.h"

class StickySlider : public SliderSlider {
 public:
  explicit StickySlider(double min = 0, double max = 100, double sticky = 50);
  void set_sticky_center(double center) { sticky_center_ = center; }
  double sticky_center() const { return sticky_center_; }
  void SnapToSticky();

 private:
  double sticky_center_ = 50;
};

#endif
