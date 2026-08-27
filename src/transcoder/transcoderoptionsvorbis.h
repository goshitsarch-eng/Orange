#ifndef STRAWBERRY_TRANSCODEROPTIONSVORBIS_H
#define STRAWBERRY_TRANSCODEROPTIONSVORBIS_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsVorbis : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::OggVorbis; }
  std::string EncoderElement() const override { return "vorbisenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Vorbis *options() { return &options_; }

 private:
  TranscoderOptionsFields::Vorbis options_;
};

#endif
