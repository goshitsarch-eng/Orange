#ifndef STRAWBERRY_TRANSCODEROPTIONSINTERFACE_H
#define STRAWBERRY_TRANSCODEROPTIONSINTERFACE_H

#include "transcoder/transcoder.h"

#include <string>

class TranscoderOptionsInterface {
 public:
  virtual ~TranscoderOptionsInterface() = default;
  virtual Transcoder::Format format() const = 0;
  virtual std::string EncoderElement() const = 0;
  virtual std::string MuxerElement() const = 0;
  virtual void ApplyQuality(int quality) = 0;
  virtual std::string PipelineFragment() const;
};

#endif
