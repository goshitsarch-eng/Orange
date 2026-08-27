#ifndef STRAWBERRY_RATINGPAINTER_H
#define STRAWBERRY_RATINGPAINTER_H

#include <string>

class RatingPainter {
 public:
  static constexpr int kStarCount = 5;

  static float RatingForPos(int x, int width);
  static int StarCount(float rating);
  static std::string Stars(float rating);
};

#endif
