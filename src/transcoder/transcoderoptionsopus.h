#ifndef STRAWBERRY_TRANSCODEROPTIONSOPUS_H
#define STRAWBERRY_TRANSCODEROPTIONSOPUS_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsOpus : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::Opus; }
  std::string EncoderElement() const override { return "opusenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Opus *options() { return &options_; }

 private:
  TranscoderOptionsFields::Opus options_;
};

#endif
