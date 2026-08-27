#ifndef STRAWBERRY_MOODBARRENDERER_H
#define STRAWBERRY_MOODBARRENDERER_H

#include <cairo.h>
#include <cstdint>
#include <vector>

class MoodbarRenderer {
 public:
  static void Draw(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood, int64_t position_nanosec = -1,
                   int64_t length_nanosec = 0, double fill_r = 1.0, double fill_g = 1.0, double fill_b = 1.0);
};

#endif
