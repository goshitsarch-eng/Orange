#include "transcoder/transcoderoptionsinterface.h"

#include <algorithm>

int TranscoderOptionsInterface::BitrateKbps(int quality, int min_kbps, int max_kbps) {
  const int clamped = std::clamp(quality, 0, 10);
  if (max_kbps <= min_kbps) {
    return min_kbps;
  }
  return min_kbps + (max_kbps - min_kbps) * clamped / 10;
}

std::string TranscoderOptionsInterface::PipelineFragment() const {
  const std::string encoder = EncoderElement();
  const std::string muxer = MuxerElement();
  if (muxer.empty()) {
    return encoder;
  }
  return encoder + " ! " + muxer;
}
