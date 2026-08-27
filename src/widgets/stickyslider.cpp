#include "widgets/stickyslider.h"

#include <cmath>

StickySlider::StickySlider(double min, double max, double sticky) : SliderSlider(min, max, 1), sticky_center_(sticky) {}

void StickySlider::SnapToSticky() {
  if (std::abs(value() - sticky_center_) < 3.0) {
    set_value(sticky_center_);
  }
}
