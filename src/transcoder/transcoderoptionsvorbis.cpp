#include "transcoder/transcoderoptionsvorbis.h"

std::string TranscoderOptionsVorbis::PipelineFragment() const {
  const double q = static_cast<double>(quality_) / 10.0;
  return "vorbisenc quality=" + std::to_string(q) + " ! oggmux";
}
