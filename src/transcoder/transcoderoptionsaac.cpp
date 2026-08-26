#include "transcoder/transcoderoptionsaac.h"

std::string TranscoderOptionsAac::PipelineFragment() const {
  const int bitrate = BitrateKbps(quality_, 64, 320) * 1000;
  return "avenc_aac bitrate=" + std::to_string(bitrate) + " ! mp4mux";
}
