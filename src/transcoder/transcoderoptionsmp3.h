#ifndef STRAWBERRY_TRANSCODEROPTIONSMP3_H
#define STRAWBERRY_TRANSCODEROPTIONSMP3_H

#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsMp3 : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::MP3; }
  std::string EncoderElement() const override { return "lamemp3enc"; }
  std::string MuxerElement() const override { return "id3v2mux"; }
  void ApplyQuality(int quality) override { options_.ApplyQuality(quality); }
  void Load() override { options_.Load(); }
  void Save() const { options_.Save(); }
  std::string PipelineFragment() const override { return options_.Pipeline(); }
  int quality() const { return options_.quality; }
  TranscoderOptionsFields::Mp3 *options() { return &options_; }

 private:
  TranscoderOptionsFields::Mp3 options_;
};

#endif
