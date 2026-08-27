#ifndef STRAWBERRY_TRANSCODEQUALITY_H
#define STRAWBERRY_TRANSCODEQUALITY_H

#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsfields.h"

namespace TranscodeQuality {

inline int Stored(Transcoder::Format format, int fallback = 5) {
  Settings settings;
  TranscoderOptionsFields::BeginStoredGroup(&settings, format);
  return settings.IntValue("quality", fallback);
}

inline int JobOverride(int spin, int stored) { return spin == stored ? -1 : spin; }

}  // namespace TranscodeQuality

#endif
