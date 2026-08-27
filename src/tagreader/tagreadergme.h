#ifndef STRAWBERRY_TAGREADERGME_H
#define STRAWBERRY_TAGREADERGME_H

#include "core/song.h"

#include <cstdint>
#include <string>

class TagReaderGME {
 public:
  static bool IsSupported(const std::string &filename);
  static bool ReadFile(const std::string &filename, Song *song);
  static bool ReadSpcData(const std::string &data, Song *song);
  static bool ReadVgmData(const std::string &data, Song *song);
  static uint32_t UnpackBytes32(const char *bytes, size_t length);
  static uint64_t ConvertSPCStringToNum(const char *bytes, size_t length);
  static bool GetPlaybackLengthMs(const char *sample_count, const char *loop_count, uint64_t *out_length_ms);

  static constexpr int kHasId6Offset = 0x23;
  static constexpr int kSongTitleOffset = 0x2E;
  static constexpr int kGameTitleOffset = 0x4E;
  static constexpr int kIntroLengthOffset = 0xA9;
  static constexpr int kIntroLengthSize = 3;
  static constexpr int kFadeLengthOffset = 0xAC;
  static constexpr int kFadeLengthSize = 4;
  static constexpr int kArtistOffset = 0xB1;
  static constexpr int kFieldSize = 32;
  static constexpr int kGd3TagPtr = 0x14;
  static constexpr int kSampleCount = 0x18;
  static constexpr int kLoopSampleCount = 0x20;
  static constexpr int kSampleTimebase = 44100;
  static constexpr int kGstGmeLoopTimeMs = 8000;
};

#endif
