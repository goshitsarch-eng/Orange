#ifndef STRAWBERRY_TRANSCODEROPTIONSSPEEX_H
#define STRAWBERRY_TRANSCODEROPTIONSSPEEX_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsSpeex : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::Speex; }
  std::string EncoderElement() const override { return "speexenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Speex *options() { return &options_; }

 private:
  TranscoderOptionsFields::Speex options_;
};

#endif
