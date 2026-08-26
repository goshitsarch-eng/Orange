#ifndef STRAWBERRY_TRANSCODEROPTIONSASF_H
#define STRAWBERRY_TRANSCODEROPTIONSASF_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsAsf : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::ASF; }
  std::string EncoderElement() const override { return "avenc_wmav2"; }
  std::string MuxerElement() const override { return "asfmux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
