#ifndef STRAWBERRY_MOODBAR_H
#define STRAWBERRY_MOODBAR_H

#include "core/signal.h"
#include "core/song.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class MoodbarLoader {
 public:
  std::vector<uint8_t> Load(const Song &song);
  std::vector<uint8_t> LoadCached(const Song &song) const;
  std::vector<uint8_t> Generate(const Song &song, bool save, const std::string &cache_dir) const;
};

class MoodbarController {
 public:
  explicit MoodbarController(MoodbarLoader *loader);
  ~MoodbarController();
  void Load(const Song &song);
  void ApplyGenerated(int generation, std::vector<uint8_t> data);
  const std::vector<uint8_t> &data() const { return data_; }
  bool busy() const { return busy_; }
  int generation() const { return generation_; }
  Signal<std::vector<uint8_t>> Ready;

 private:
  MoodbarLoader *loader_;
  std::vector<uint8_t> data_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  int generation_ = 0;
  bool busy_ = false;
};

#endif
