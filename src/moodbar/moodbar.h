#ifndef STRAWBERRY_MOODBAR_H
#define STRAWBERRY_MOODBAR_H
#include "core/song.h"
#include "core/signal.h"
#include <cstdint>
#include <string>
#include <vector>
class MoodbarLoader {
 public:
  std::vector<uint8_t> Load(const Song &song);
};
class MoodbarController {
 public:
  explicit MoodbarController(MoodbarLoader *loader);
  void Load(const Song &song);
  const std::vector<uint8_t> &data() const { return data_; }
  Signal<std::vector<uint8_t>> Ready;
 private:
  MoodbarLoader *loader_;
  std::vector<uint8_t> data_;
};
#endif
