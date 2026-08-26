#include "transcoder/transcoderoptionsflac.h"

std::string TranscoderOptionsFlac::PipelineFragment() const {
  return "flacenc quality=" + std::to_string(quality_);
}
