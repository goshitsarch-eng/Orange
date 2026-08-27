#ifndef STRAWBERRY_RATINGWIDGETKEYS_H
#define STRAWBERRY_RATINGWIDGETKEYS_H

#include "widgets/listboxkeyboard.h"
#include "widgets/ratingpainter.h"

#include <algorithm>

namespace RatingWidgetKeys {

constexpr unsigned kKP0 = 0xffb0;
constexpr unsigned kKP9 = 0xffb9;

// Qt RatingWidget::keyPressEvent: 0.5 / kStarCount per arrow.
inline float ArrowStep(int star_count = RatingPainter::kStarCount) {
  return 0.5f / static_cast<float>(star_count > 0 ? star_count : RatingPainter::kStarCount);
}

inline float Clamp(float rating) { return std::clamp(rating, 0.0f, 1.0f); }

inline int Digit(unsigned keyval) {
  if (keyval >= '0' && keyval <= '9') {
    return static_cast<int>(keyval - '0');
  }
  if (keyval >= kKP0 && keyval <= kKP9) {
    return static_cast<int>(keyval - kKP0);
  }
  return -1;
}

inline float FromDigit(unsigned keyval, int star_count = RatingPainter::kStarCount) {
  const int digit = Digit(keyval);
  if (digit < 0) {
    return -1.0f;
  }
  return Clamp(static_cast<float>(digit) / static_cast<float>(star_count > 0 ? star_count : RatingPainter::kStarCount));
}

inline float FromArrow(unsigned keyval, float current, int star_count = RatingPainter::kStarCount) {
  const float step = ArrowStep(star_count);
  if (keyval == ListBoxKeyboard::kLeft) {
    return Clamp(current - step);
  }
  if (keyval == ListBoxKeyboard::kRight) {
    return Clamp(current + step);
  }
  return -1.0f;
}

// Qt: digits 0–9 set rating; Left/Right nudge by one half-star (0.1 with 5 stars).
inline float FromKey(unsigned keyval, float current, int star_count = RatingPainter::kStarCount) {
  const float digit = FromDigit(keyval, star_count);
  if (digit >= 0.0f) {
    return digit;
  }
  return FromArrow(keyval, current, star_count);
}

inline bool ShouldApply(float next, float current) { return next >= 0.0f && next != current; }

}  // namespace RatingWidgetKeys

#endif
