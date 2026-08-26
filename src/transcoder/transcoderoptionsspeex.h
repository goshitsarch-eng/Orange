#ifndef STRAWBERRY_TRANSCODEROPTIONSSPEEX_H
#define STRAWBERRY_TRANSCODEROPTIONSSPEEX_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsSpeex : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::Speex; }
  std::string EncoderElement() const override { return "speexenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  std::string PipelineFragment() const override;
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
