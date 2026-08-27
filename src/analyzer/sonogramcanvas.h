#ifndef STRAWBERRY_SONOGRAMCANVAS_H
#define STRAWBERRY_SONOGRAMCANVAS_H

#include <algorithm>
#include <cstdint>
#include <vector>

namespace SonogramCanvas {

inline constexpr uint32_t kBackground = 0x00202020;
inline constexpr uint32_t kHot = 0x00FF0000;

inline uint32_t Hsv(int h, int s, int v) {
  h = std::clamp(h, 0, 359);
  s = std::clamp(s, 0, 255);
  v = std::clamp(v, 0, 255);
  const int region = h / 60;
  const int remainder = (h - region * 60) * 255 / 60;
  const int p = v * (255 - s) / 255;
  const int q = v * (255 - s * remainder / 255) / 255;
  const int t = v * (255 - s * (255 - remainder) / 255) / 255;
  int r = 0;
  int g = 0;
  int b = 0;
  switch (region) {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default:
      r = v;
      g = p;
      b = q;
      break;
  }
  return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

inline uint32_t ColorFromValue(float value) {
  if (value < 0.005f) {
    return kBackground;
  }
  if (value < 0.05f) {
    return Hsv(95, 255, 255 - static_cast<int>(value * 4000.0f));
  }
  if (value < 1.0f) {
    return Hsv(95 - static_cast<int>(value * 90.0f), 255, 255);
  }
  return kHot;
}

struct Buffer {
  std::vector<uint32_t> pixels;
  int width = 0;
  int height = 0;

  void EnsureSize(int w, int h) {
    if (w < 1) {
      w = 1;
    }
    if (h < 1) {
      h = 1;
    }
    if (w == width && h == height && static_cast<int>(pixels.size()) == w * h) {
      return;
    }
    width = w;
    height = h;
    pixels.assign(static_cast<size_t>(w * h), kBackground);
  }

  uint32_t At(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) {
      return kBackground;
    }
    return pixels[static_cast<size_t>(y * width + x)];
  }

  void ShiftLeft() {
    if (width <= 1) {
      return;
    }
    for (int y = 0; y < height; ++y) {
      const int row = y * width;
      for (int x = 0; x + 1 < width; ++x) {
        pixels[static_cast<size_t>(row + x)] = pixels[static_cast<size_t>(row + x + 1)];
      }
      pixels[static_cast<size_t>(row + width - 1)] = kBackground;
    }
  }

  void WriteRightColumn(const std::vector<float> &spectrum) {
    if (width < 1 || height < 1) {
      return;
    }
    const int x = width - 1;
    for (int y = 0; y < height; ++y) {
      const int src = height - 1 - y;
      const float value = src >= 0 && static_cast<size_t>(src) < spectrum.size() ? spectrum[static_cast<size_t>(src)] : 0.0f;
      pixels[static_cast<size_t>(y * width + x)] = ColorFromValue(value);
    }
  }
};

}  // namespace SonogramCanvas

#endif  // STRAWBERRY_SONOGRAMCANVAS_H
