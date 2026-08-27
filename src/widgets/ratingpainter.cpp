#include "widgets/ratingpainter.h"

#include <algorithm>
#include <cmath>

float RatingPainter::RatingForPos(int x, int width) {
  if (width <= 0) {
    return -1.0f;
  }
  const float ratio = std::clamp(static_cast<float>(x) / static_cast<float>(width), 0.0f, 1.0f);
  const int stars = static_cast<int>(std::lround(ratio * kStarCount));
  if (stars <= 0) {
    return 0.0f;
  }
  return static_cast<float>(stars) / static_cast<float>(kStarCount);
}

int RatingPainter::StarCount(float rating) {
  if (rating < 0.0f) {
    return 0;
  }
  return std::clamp(static_cast<int>(std::lround(rating * kStarCount)), 0, kStarCount);
}

std::string RatingPainter::Stars(float rating) {
  if (rating < 0.0f) {
    return {};
  }
  std::string text;
  const int filled = StarCount(rating);
  for (int i = 0; i < kStarCount; ++i) {
    text += (i < filled) ? "★" : "☆";
  }
  return text;
}
