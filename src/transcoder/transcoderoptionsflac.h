#ifndef STRAWBERRY_TRANSCODEROPTIONSFLAC_H
#define STRAWBERRY_TRANSCODEROPTIONSFLAC_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsFlac : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::FLAC; }
  std::string EncoderElement() const override { return "flacenc"; }
  std::string MuxerElement() const override { return {}; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  void Load() override;
  std::string PipelineFragment() const override;
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
