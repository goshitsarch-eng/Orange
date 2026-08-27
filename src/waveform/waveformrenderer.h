#ifndef STRAWBERRY_WAVEFORMRENDERER_H
#define STRAWBERRY_WAVEFORMRENDERER_H

#include <cairo.h>
#include <cstdint>
#include <vector>

class WaveformRenderer {
 public:
  static void Draw(cairo_t *cr, int width, int height, const std::vector<float> &peaks, int64_t position_nanosec = -1,
                   int64_t length_nanosec = 0, double cursor_r = 0.0, double cursor_g = 0.0, double cursor_b = 0.0);
};

#endif
