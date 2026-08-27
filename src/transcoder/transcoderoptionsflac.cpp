#include "transcoder/transcoderoptionsflac.h"

#include "transcoder/transcoderoptionsfields.h"

void TranscoderOptionsFlac::Load() {
  TranscoderOptionsFields::QualityEncoder encoder;
  encoder.Load(Transcoder::Format::FLAC);
  quality_ = encoder.quality;
}

std::string TranscoderOptionsFlac::PipelineFragment() const {
  return "flacenc quality=" + std::to_string(quality_);
}
