#include "transcoder/transcoderoptionsopus.h"

std::string TranscoderOptionsOpus::PipelineFragment() const {
  const int bitrate = BitrateKbps(quality_, 48, 256) * 1000;
  return "opusenc bitrate=" + std::to_string(bitrate) + " ! oggmux";
}
