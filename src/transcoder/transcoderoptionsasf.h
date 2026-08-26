#ifndef STRAWBERRY_TRANSCODEROPTIONSASF_H
#define STRAWBERRY_TRANSCODEROPTIONSASF_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsAsf : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::ASF; }
  std::string EncoderElement() const override { return "avenc_wmav2"; }
  std::string MuxerElement() const override { return "asfmux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Asf *options() { return &options_; }

 private:
  TranscoderOptionsFields::Asf options_;
};

#endif
