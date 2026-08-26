#ifndef STRAWBERRY_TRANSCODEROPTIONSAAC_H
#define STRAWBERRY_TRANSCODEROPTIONSAAC_H

#include "transcoder/transcoderoptionsinterface.h"

class TranscoderOptionsAac : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::AAC; }
  std::string EncoderElement() const override { return "avenc_aac"; }
  std::string MuxerElement() const override { return "mp4mux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  std::string PipelineFragment() const override;
  int quality() const { return quality_; }

 private:
  int quality_ = 5;
};

#endif
