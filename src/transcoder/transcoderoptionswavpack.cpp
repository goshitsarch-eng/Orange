#include "transcoder/transcoderoptionswavpack.h"

std::string TranscoderOptionsWavPack::PipelineFragment() const {
  const int mode = quality_ >= 8 ? 2 : (quality_ >= 4 ? 1 : 0);
  return "wavpackenc mode=" + std::to_string(mode);
}
