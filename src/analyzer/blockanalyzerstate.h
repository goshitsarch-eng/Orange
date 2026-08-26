#ifndef STRAWBERRY_BLOCKANALYZERSTATE_H
#define STRAWBERRY_BLOCKANALYZERSTATE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// Falling bars, fade trails, and top-cap matching Qt BlockAnalyzer (analyzer/blockanalyzer.cpp).
struct BlockAnalyzerState {
  static constexpr int kHeight = 2;
  static constexpr int kWidth = 4;
  static constexpr int kFadeSize = 90;

  int rows = 0;
  int y_offset = 0;
  double step = 1.0;
  std::vector<double> store;
  std::vector<double> yscale;
  std::vector<int> fade_pos;
  std::vector<int> fade_intensity;

  void Ensure(size_t columns, int height, int timeout_ms = 40) {
    const int new_rows = std::max(1, (height + 1) / (kHeight + 1));
    y_offset = (height - (new_rows * (kHeight + 1)) + 2) / 2;
    step = timeout_ms < 20 ? static_cast<double>(timeout_ms) / 20.0 : static_cast<double>(timeout_ms) / 30.0;
    if (rows == new_rows && store.size() == columns && yscale.size() == static_cast<size_t>(new_rows + 1)) {
      return;
    }
    rows = new_rows;
    store.assign(columns, static_cast<double>(rows));
    fade_pos.assign(columns, rows);
    fade_intensity.assign(columns, 0);
    yscale.resize(static_cast<size_t>(rows + 1));
    for (int z = 0; z < rows; ++z) {
      yscale[static_cast<size_t>(z)] = 1.0 - (std::log10(1 + z) / std::log10(1 + rows + 1));
    }
    yscale[static_cast<size_t>(rows)] = 0.0;
  }

  int RowFor(float scope) const {
    int y = 0;
    while (y < rows && scope < static_cast<float>(yscale[static_cast<size_t>(y)])) {
      ++y;
    }
    return y;
  }

  void Advance(const std::vector<float> &bands, int height, int timeout_ms = 40) {
    Ensure(bands.size(), height, timeout_ms);
    for (size_t x = 0; x < bands.size(); ++x) {
      int y = RowFor(bands[x]);
      if (static_cast<double>(y) > store[x]) {
        store[x] += step;
        y = static_cast<int>(store[x]);
      }
      else {
        store[x] = static_cast<double>(y);
      }
      if (y <= fade_pos[x]) {
        fade_pos[x] = y;
        fade_intensity[x] = kFadeSize;
      }
      if (fade_intensity[x] > 0) {
        --fade_intensity[x];
      }
      if (fade_intensity[x] == 0) {
        fade_pos[x] = rows;
      }
    }
  }
};

#endif
