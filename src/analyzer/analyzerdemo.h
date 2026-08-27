#ifndef STRAWBERRY_ANALYZERDEMO_H
#define STRAWBERRY_ANALYZERDEMO_H

#include <cmath>
#include <cstddef>
#include <vector>

// Idle sine sweep matching Qt AnalyzerBase::demo (analyzer/analyzerbase.cpp).
struct AnalyzerDemo {
  static constexpr int kPeriod = 1000;
  static constexpr int kActiveUntil = 201;
  static constexpr double kPi = 3.14159265358979323846;

  int t = kActiveUntil;

  std::vector<float> Next(size_t n) {
    if (t > kPeriod - 1) {
      t = 1;
    }
    std::vector<float> scope(n, 0.0f);
    if (t < kActiveUntil && n > 0) {
      const double dt = static_cast<double>(t) / 200.0;
      for (size_t i = 0; i < n; ++i) {
        scope[i] = static_cast<float>(dt * (std::sin(kPi + (static_cast<double>(i) * kPi) / static_cast<double>(n)) + 1.0));
      }
    }
    ++t;
    return scope;
  }
};

#endif
