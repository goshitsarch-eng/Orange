#ifndef STRAWBERRY_TRANSCODEROPTIONSVORBIS_H
#define STRAWBERRY_TRANSCODEROPTIONSVORBIS_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsVorbis : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::OggVorbis; }
  std::string EncoderElement() const override { return "vorbisenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  std::string PipelineFragment() const override;
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
