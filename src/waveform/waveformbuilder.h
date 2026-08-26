#ifndef STRAWBERRY_WAVEFORMBUILDER_H
#define STRAWBERRY_WAVEFORMBUILDER_H

#include <cstdint>
#include <vector>

class WaveformBuilder {
 public:
  static std::vector<float> FromPcm(const int16_t *samples, size_t count, size_t channels = 1, size_t bins = 512);
};

#endif
