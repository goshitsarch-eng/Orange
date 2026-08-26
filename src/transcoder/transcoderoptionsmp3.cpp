#include "transcoder/transcoderoptionsmp3.h"

std::string TranscoderOptionsMp3::PipelineFragment() const {
  const int bitrate = 96 + quality_ * 16;
  return "lamemp3enc target=1 bitrate=" + std::to_string(bitrate) + " ! xingmux ! id3v2mux";
}
