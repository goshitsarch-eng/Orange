#ifndef STRAWBERRY_MOODBARBUILDER_H
#define STRAWBERRY_MOODBARBUILDER_H

#include <cstdint>
#include <vector>

class MoodbarBuilder {
 public:
  static std::vector<uint8_t> FromPcm(const int16_t *samples, size_t count, size_t channels = 1, size_t bins = 300);
};

#endif
