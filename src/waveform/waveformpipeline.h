#ifndef STRAWBERRY_WAVEFORMPIPELINE_H
#define STRAWBERRY_WAVEFORMPIPELINE_H

#include <string>
#include <vector>

class WaveformPipeline {
 public:
  static std::vector<float> Run(const std::string &url);
};

#endif
