#ifndef STRAWBERRY_STREAMINGCHOICES_H
#define STRAWBERRY_STREAMINGCHOICES_H

#include <string>
#include <utility>
#include <vector>

namespace StreamingChoices {

inline const std::vector<std::pair<std::string, std::string>> &TidalQualities() {
  static const std::vector<std::pair<std::string, std::string>> choices = {
      {"LOW", "Low"}, {"HIGH", "High"}, {"LOSSLESS", "Lossless"}, {"HI_RES", "Hi-Res"}, {"HI_RES_LOSSLESS", "Hi-Res Lossless"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &TidalStreamUrlMethods() {
  static const std::vector<std::pair<std::string, std::string>> choices = {
      {"0", "Stream URL"}, {"1", "URL post-paywall"}, {"2", "Playback info post-paywall"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &TidalCoverSizes() {
  static const std::vector<std::pair<std::string, std::string>> choices = {
      {"160x160", "160x160"}, {"320x320", "320x320"}, {"640x640", "640x640"}, {"750x750", "750x750"}, {"1280x1280", "1280x1280"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &QobuzFormats() {
  static const std::vector<std::pair<std::string, std::string>> choices = {
      {"5", "MP3 320"}, {"6", "FLAC 16-bit"}, {"7", "FLAC 24-bit 96 kHz"}, {"27", "FLAC 24-bit 192 kHz"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &SubsonicAuthMethods() {
  static const std::vector<std::pair<std::string, std::string>> choices = {{"0", "Hex"}, {"1", "MD5 token (Recommended)"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &SomaFmQualities() {
  static const std::vector<std::pair<std::string, std::string>> choices = {{"highest", "Highest"}, {"high", "High"}, {"low", "Low"}};
  return choices;
}

inline const std::vector<std::pair<std::string, std::string>> &RadioParadiseStreams() {
  static const std::vector<std::pair<std::string, std::string>> choices = {
      {"aac-320", "AAC 320"}, {"aac-128", "AAC 128"}, {"mp3-192", "MP3 192"}};
  return choices;
}

}  // namespace StreamingChoices

#endif
