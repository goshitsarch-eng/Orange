#ifndef STRAWBERRY_MOODBARPIPELINE_H
#define STRAWBERRY_MOODBARPIPELINE_H

#include <cstdint>
#include <string>
#include <vector>

class MoodbarPipeline {
 public:
  static std::vector<uint8_t> Run(const std::string &url);
};

#endif
