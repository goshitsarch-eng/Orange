#ifndef STRAWBERRY_TRANSCODEROPTIONSMP3_H
#define STRAWBERRY_TRANSCODEROPTIONSMP3_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsMp3 : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::MP3; }
  std::string EncoderElement() const override { return "lamemp3enc"; }
  std::string MuxerElement() const override { return "id3v2mux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  std::string PipelineFragment() const override;
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
