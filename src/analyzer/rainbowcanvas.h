#ifndef STRAWBERRY_RAINBOWCANVAS_H
#define STRAWBERRY_RAINBOWCANVAS_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <vector>

// Scrolling 6-band history matching Qt RainbowAnalyzer (analyzer/rainbowanalyzer.cpp).
struct RainbowCanvas {
  static constexpr int kBands = 6;
  static constexpr int kHistorySize = 128;
  static constexpr float kPixelScale = 0.02F;
  static constexpr double kPi = 3.14159265358979323846;

  std::vector<float> history;

  static float BandScale(int band) {
    return -static_cast<float>(std::cos(kPi * band / (kBands - 1))) * 0.5F * static_cast<float>(std::pow(2.3, band));
  }

  void Ensure() {
    if (static_cast<int>(history.size()) != kBands * kHistorySize) {
      history.assign(static_cast<size_t>(kBands * kHistorySize), 0.0f);
    }
  }

  float At(int band, int x) const {
    if (band < 0 || band >= kBands || x < 0 || x >= kHistorySize || history.empty()) {
      return 0.0f;
    }
    return history[static_cast<size_t>(band * kHistorySize + x)];
  }

  float Last(int band) const { return At(band, kHistorySize - 1); }

  void Advance(const std::vector<float> &bands) {
    Ensure();
    for (int band = 0; band < kBands; ++band) {
      float *start = history.data() + band * kHistorySize;
      std::memmove(start, start + 1, static_cast<size_t>(kHistorySize - 1) * sizeof(float));
    }
    const int samples_per_band = std::max(1, static_cast<int>(bands.size()) / kBands);
    size_t sample = 0;
    for (int band = 0; band < kBands; ++band) {
      float accumulator = 0.0f;
      for (int i = 0; i < samples_per_band && sample < bands.size(); ++i) {
        accumulator += bands[sample++];
      }
      history[static_cast<size_t>((band + 1) * kHistorySize - 1)] = accumulator * BandScale(band);
    }
  }

  static void ColorForBand(int band, double *r, double *g, double *b) {
    const double t = kBands <= 1 ? 0.0 : static_cast<double>(band) / static_cast<double>(kBands - 1);
    *r = t;
    *g = 1.0 - std::abs(t - 0.5) * 2.0;
    *b = 1.0 - t;
  }
};

#endif
