#ifndef STRAWBERRY_TRANSCODEROPTIONSOPUS_H
#define STRAWBERRY_TRANSCODEROPTIONSOPUS_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsOpus : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::Opus; }
  std::string EncoderElement() const override { return "opusenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
