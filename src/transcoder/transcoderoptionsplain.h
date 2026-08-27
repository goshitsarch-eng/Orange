#ifndef STRAWBERRY_TRANSCODEROPTIONSPLAIN_H
#define STRAWBERRY_TRANSCODEROPTIONSPLAIN_H

#include "transcoder/transcoderoptionsinterface.h"

#include <utility>

class TranscoderOptionsPlain : public TranscoderOptionsInterface {
 public:
  TranscoderOptionsPlain(Transcoder::Format format, std::string encoder, std::string muxer = {})
      : format_(format), encoder_(std::move(encoder)), muxer_(std::move(muxer)) {}

  Transcoder::Format format() const override { return format_; }
  std::string EncoderElement() const override { return encoder_; }
  std::string MuxerElement() const override { return muxer_; }
  void ApplyQuality(int) override {}

 private:
  Transcoder::Format format_ = Transcoder::Format::WAV;
  std::string encoder_;
  std::string muxer_;
};

class TranscoderOptionsOggFlac : public TranscoderOptionsInterface {
 public:
  Transcoder::Format format() const override { return Transcoder::Format::OggFlac; }
  std::string EncoderElement() const override { return "flacenc"; }
  std::string MuxerElement() const override { return "oggmux"; }
  void ApplyQuality(int quality) override { quality_ = quality; }
  void Load() override;
  std::string PipelineFragment() const override;

 private:
  int quality_ = 5;
};

#endif
