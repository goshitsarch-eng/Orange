#ifndef STRAWBERRY_WAVEAMPLITUDE_H
#define STRAWBERRY_WAVEAMPLITUDE_H

#include <algorithm>
#include <cmath>

// Amplitude coloring matching Qt WaveRubberAnalyzer (analyzer/waverubberanalyzer.cpp).
struct WaveAmplitude {
  static constexpr double kHighlightG = 0.63;
  static constexpr double kHighlightB = 0.95;

  static void Color(float sample, double *r, double *g, double *b) {
    const float color_factor = sample / 2.0F + 0.5F;
    *r = std::clamp(1.0 - static_cast<double>(color_factor), 0.0, 1.0);
    *g = kHighlightG;
    *b = kHighlightB;
  }

  // Qt WaveRubber draws about the upper quarter, then offsets the polyline by mid_y.
  static int MidY(int height) { return std::max(1, height / 4); }

  static double DrawY(float sample, int mid_y) {
    return static_cast<double>(mid_y) - static_cast<double>(sample) * static_cast<double>(mid_y) + static_cast<double>(mid_y);
  }
};

#endif
