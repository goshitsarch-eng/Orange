#include "transcoder/transcoderoptionsinterface.h"

std::string TranscoderOptionsInterface::PipelineFragment() const {
  const std::string encoder = EncoderElement();
  const std::string muxer = MuxerElement();
  if (muxer.empty()) {
    return encoder;
  }
  return encoder + " ! " + muxer;
}
