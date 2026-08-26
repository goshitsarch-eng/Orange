#ifndef STRAWBERRY_TRAYICONPIXMAP_H
#define STRAWBERRY_TRAYICONPIXMAP_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace TrayIconPixmap {

inline constexpr int kDefaultSize = 48;

inline bool ValidDimensions(int width, int height) { return width > 0 && height > 0 && width <= 256 && height <= 256; }

inline size_t ByteCount(int width, int height) {
  if (!ValidDimensions(width, height)) {
    return 0;
  }
  return static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
}

inline void WriteNetworkArgb(uint8_t *out, uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  if (!out) {
    return;
  }
  out[0] = a;
  out[1] = r;
  out[2] = g;
  out[3] = b;
}

inline void AppendNetworkArgb(std::vector<uint8_t> *out, uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  if (!out) {
    return;
  }
  out->push_back(a);
  out->push_back(r);
  out->push_back(g);
  out->push_back(b);
}

inline uint32_t NativeArgb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

inline void PackNativeArgbRow(const uint32_t *src, int width, std::vector<uint8_t> *out) {
  if (!src || !out || width <= 0) {
    return;
  }
  for (int x = 0; x < width; ++x) {
    const uint32_t pixel = src[x];
    AppendNetworkArgb(out, static_cast<uint8_t>((pixel >> 24) & 0xff), static_cast<uint8_t>((pixel >> 16) & 0xff),
                      static_cast<uint8_t>((pixel >> 8) & 0xff), static_cast<uint8_t>(pixel & 0xff));
  }
}

}  // namespace TrayIconPixmap

#endif  // STRAWBERRY_TRAYICONPIXMAP_H
