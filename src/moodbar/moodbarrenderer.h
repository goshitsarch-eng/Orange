#ifndef STRAWBERRY_MOODBARRENDERER_H
#define STRAWBERRY_MOODBARRENDERER_H

#include <cairo.h>
#include <cstdint>
#include <vector>

class MoodbarRenderer {
 public:
  static void Draw(cairo_t *cr, int width, int height, const std::vector<uint8_t> &mood);
};

#endif
