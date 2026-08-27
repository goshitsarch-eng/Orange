#ifndef STRAWBERRY_TRANSCODEROPTIONSAAC_H
#define STRAWBERRY_TRANSCODEROPTIONSAAC_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsAac : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::AAC; }
  std::string EncoderElement() const override { return "avenc_aac"; }
  std::string MuxerElement() const override { return "mp4mux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Aac *options() { return &options_; }

 private:
  TranscoderOptionsFields::Aac options_;
};

#endif
