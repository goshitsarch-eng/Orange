#ifndef STRAWBERRY_TRANSCODEROPTIONSWAVPACK_H
#define STRAWBERRY_TRANSCODEROPTIONSWAVPACK_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsWavPack : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::WavPack; }
  std::string EncoderElement() const override { return "wavpackenc"; }
  std::string MuxerElement() const override { return {}; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
