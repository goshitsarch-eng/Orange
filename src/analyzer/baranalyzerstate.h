#ifndef STRAWBERRY_BARANALYZERSTATE_H
#define STRAWBERRY_BARANALYZERSTATE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// Peak-hold and decay matching Qt BoomAnalyzer / TurbineAnalyzer (analyzer/boomanalyzer.cpp).
struct BarAnalyzerState {
  static constexpr double kBarHeight = 1.271;
  static constexpr double kPeakAccel = 1.103;
  static constexpr double kInitialPeakSpeed = 0.01;

  std::vector<double> bar_height;
  std::vector<double> peak_height;
  std::vector<double> peak_speed;

  void Resize(size_t n) {
    if (bar_height.size() == n) {
      return;
    }
    bar_height.assign(n, 0.0);
    peak_height.assign(n, 0.0);
    peak_speed.assign(n, kInitialPeakSpeed);
  }

  // Qt resizeEvent: F_ = (height - 2) / (log10(256) * 1.1).
  static double FScale(int pixel_height) {
    const int usable = std::max(1, pixel_height - 2);
    return static_cast<double>(usable) / (std::log10(256.0) * 1.1);
  }

  static double TargetHeight(double sample, double f, double max_height) {
    if (sample <= 0.0 || max_height <= 0.0) {
      return 0.0;
    }
    return std::min(std::log10(sample * 256.0) * f, max_height);
  }

  void Advance(const std::vector<float> &bands, int pixel_height, double f_mul = 1.0, double max_height = -1.0) {
    Resize(bands.size());
    const double f = FScale(pixel_height) * f_mul;
    const double cap = max_height >= 0.0 ? max_height : static_cast<double>(std::max(0, pixel_height - 1));
    for (size_t i = 0; i < bands.size(); ++i) {
      const double h = TargetHeight(bands[i], f, cap);
      if (h > bar_height[i]) {
        bar_height[i] = h;
        if (h > peak_height[i]) {
          peak_height[i] = h;
          peak_speed[i] = kInitialPeakSpeed;
          continue;
        }
      }
      else if (bar_height[i] > 0.0) {
        bar_height[i] = std::max(0.0, bar_height[i] - kBarHeight);
      }
      if (peak_height[i] > 0.0) {
        peak_height[i] -= peak_speed[i];
        peak_speed[i] *= kPeakAccel;
        peak_height[i] = std::max(0.0, std::max(bar_height[i], peak_height[i]));
      }
    }
  }
};

#endif
