#ifndef STRAWBERRY_WAVEFORMRENDERER_H
#define STRAWBERRY_WAVEFORMRENDERER_H

#include <cairo.h>
#include <vector>

class WaveformRenderer {
 public:
  static void Draw(cairo_t *cr, int width, int height, const std::vector<float> &peaks);
};

#endif
