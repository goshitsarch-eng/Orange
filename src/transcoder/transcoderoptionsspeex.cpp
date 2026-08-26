#include "transcoder/transcoderoptionsspeex.h"

#include <algorithm>

std::string TranscoderOptionsSpeex::PipelineFragment() const {
  return "speexenc quality=" + std::to_string(std::clamp(quality_, 0, 10)) + " ! oggmux";
}
