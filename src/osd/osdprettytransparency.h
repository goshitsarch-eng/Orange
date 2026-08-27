#ifndef STRAWBERRY_OSDPRETTYTRANSPARENCY_H
#define STRAWBERRY_OSDPRETTYTRANSPARENCY_H

#include <algorithm>
#include <vector>

namespace OSDPrettyTransparency {

// Qt osdpretty.cpp kBorderRadius; CSS border-radius: 10px.
inline constexpr int kBorderRadius = 10;

// Qt OSDPretty::IsTransparencyAvailable: X11 needs a compositor; Wayland always composites.
inline bool Available(bool is_x11, bool compositing_enabled, bool is_wayland) {
  if (is_x11) {
    return compositing_enabled;
  }
  if (is_wayland) {
    return true;
  }
  return true;
}

inline bool ShouldApplyShape(bool is_x11, bool transparency_available) { return is_x11 && !transparency_available; }

inline bool ShouldClearShape(bool is_x11, bool transparency_available) { return is_x11 && transparency_available; }

inline int ClampRadius(int width, int height, int radius = kBorderRadius) {
  if (width <= 0 || height <= 0) {
    return 0;
  }
  return std::max(0, std::min({radius, width / 2, height / 2}));
}

inline int MaskStride(int width) { return width <= 0 ? 0 : (width + 7) / 8; }

inline int MaskByteCount(int width, int height) {
  if (width <= 0 || height <= 0) {
    return 0;
  }
  return MaskStride(width) * height;
}

inline bool PixelInsideRoundedRect(int x, int y, int width, int height, int radius) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return false;
  }
  const int r = ClampRadius(width, height, radius);
  if (r <= 0) {
    return true;
  }
  if (x >= r && x < width - r) {
    return true;
  }
  if (y >= r && y < height - r) {
    return true;
  }
  const int cx = x < r ? r : width - 1 - r;
  const int cy = y < r ? r : height - 1 - r;
  const int dx = x - cx;
  const int dy = y - cy;
  return dx * dx + dy * dy <= r * r;
}

// XCreateBitmapFromData: LSB-first bits, rows padded to 8 bits.
inline std::vector<unsigned char> RoundedMaskBits(int width, int height, int radius = kBorderRadius) {
  std::vector<unsigned char> bits(static_cast<size_t>(MaskByteCount(width, height)), 0);
  if (bits.empty()) {
    return bits;
  }
  const int stride = MaskStride(width);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (!PixelInsideRoundedRect(x, y, width, height, radius)) {
        continue;
      }
      bits[static_cast<size_t>(y * stride + x / 8)] |= static_cast<unsigned char>(1u << (x % 8));
    }
  }
  return bits;
}

}  // namespace OSDPrettyTransparency

#endif
