#ifndef STRAWBERRY_PRETTYSLIDER_H
#define STRAWBERRY_PRETTYSLIDER_H

#include "widgets/sliderslider.h"

#include <string>

class PrettySlider : public SliderSlider {
 public:
  explicit PrettySlider(double min = 0, double max = 1000, double step = 1);
  void SetPopupText(const std::string &text);
  const std::string &popup_text() const { return popup_text_; }

 private:
  std::string popup_text_;
};

#endif
