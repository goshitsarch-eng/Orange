#include "transcoder/transcoderoptionsasf.h"

std::string TranscoderOptionsAsf::PipelineFragment() const {
  const int bitrate = BitrateKbps(quality_, 64, 192) * 1000;
  return "avenc_wmav2 bitrate=" + std::to_string(bitrate) + " ! asfmux";
}
