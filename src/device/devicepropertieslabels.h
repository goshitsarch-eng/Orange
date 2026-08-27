#ifndef STRAWBERRY_DEVICEPROPERTIESLABELS_H
#define STRAWBERRY_DEVICEPROPERTIESLABELS_H

#include "core/song.h"
#include "device/devicedatabasebackend.h"
#include "transcoder/transcoder.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace DevicePropertiesLabels {

inline const char *Title() { return "Device properties"; }
inline const char *InformationTab() { return "Information"; }
inline const char *FileFormatsTab() { return "File formats"; }
inline const char *Name() { return "Name"; }
inline const char *Icon() { return "Icon"; }
inline const char *HardwareInformation() { return "Hardware information"; }
inline const char *NotConnected() { return "Hardware information is only available while the device is connected."; }
inline const char *OpenDevice() { return "Open device"; }
inline const char *FormatsNotConnected() {
  return "This device must be connected and opened before Strawberry can see what file formats it supports.";
}
inline const char *TranscodeIntro() {
  return "Strawberry can automatically convert the music you copy to this device into a format that it can play.";
}
inline const char *Never() { return "Do not convert any music"; }
inline const char *Unsupported() { return "Convert any music that the device can't play"; }
inline const char *Always() { return "Convert all music"; }
inline const char *PreferredFormat() { return "Preferred format"; }
inline const char *SupportedFormats() { return "Supported formats"; }
inline const char *SupportedFormatsIntro() { return "This device supports the following file formats:"; }
inline const char *QueryingDevice() { return "Querying device..."; }
inline const char *Save() { return "Save"; }

inline bool UnsupportedEnabled(const bool has_supported) { return has_supported; }

inline bool SupportedListVisible(const bool has_supported) { return has_supported; }

inline bool ShouldFallbackToNever(const bool unsupported_checked, const bool has_supported) {
  return unsupported_checked && !has_supported;
}

inline bool ShouldPickBestFormat(const bool has_saved_format, const Song::FileType stored) {
  return !has_saved_format || stored == Song::FileType::Unknown;
}

inline std::vector<std::string> SupportedFormatNames(const std::vector<Song::FileType> &types) {
  std::vector<std::string> names;
  names.reserve(types.size());
  for (Song::FileType type : types) {
    names.push_back(Song::FiletypeToString(type));
  }
  std::sort(names.begin(), names.end());
  return names;
}

inline int RadioIndex(DeviceDatabaseBackend::TranscodeMode mode) {
  switch (mode) {
    case DeviceDatabaseBackend::TranscodeMode::Transcode_Never:
      return 0;
    case DeviceDatabaseBackend::TranscodeMode::Transcode_Always:
      return 2;
    case DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported:
    default:
      return 1;
  }
}

inline DeviceDatabaseBackend::TranscodeMode ModeFromRadio(int index) {
  switch (index) {
    case 0:
      return DeviceDatabaseBackend::TranscodeMode::Transcode_Never;
    case 2:
      return DeviceDatabaseBackend::TranscodeMode::Transcode_Always;
    case 1:
    default:
      return DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported;
  }
}

inline Song::FileType FileTypeFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::MP3:
      return Song::FileType::MPEG;
    case Transcoder::Format::AAC:
      return Song::FileType::MP4;
    case Transcoder::Format::FLAC:
      return Song::FileType::FLAC;
    case Transcoder::Format::OggVorbis:
      return Song::FileType::OggVorbis;
    case Transcoder::Format::Opus:
      return Song::FileType::OggOpus;
    case Transcoder::Format::Speex:
      return Song::FileType::OggSpeex;
    case Transcoder::Format::WavPack:
      return Song::FileType::WavPack;
    case Transcoder::Format::ASF:
      return Song::FileType::ASF;
    case Transcoder::Format::WAV:
      return Song::FileType::WAV;
    case Transcoder::Format::OggFlac:
      return Song::FileType::OggFlac;
    case Transcoder::Format::ALAC:
      return Song::FileType::ALAC;
  }
  return Song::FileType::FLAC;
}

inline std::vector<std::pair<Song::FileType, std::string>> FormatChoices() {
  static const Transcoder::Format kFormats[] = {
      Transcoder::Format::FLAC,     Transcoder::Format::MP3,      Transcoder::Format::AAC,     Transcoder::Format::OggVorbis,
      Transcoder::Format::Opus,     Transcoder::Format::Speex,    Transcoder::Format::WavPack, Transcoder::Format::ASF,
      Transcoder::Format::WAV,      Transcoder::Format::OggFlac,  Transcoder::Format::ALAC};
  std::vector<std::pair<Song::FileType, std::string>> out;
  out.reserve(11);
  for (Transcoder::Format format : kFormats) {
    out.emplace_back(FileTypeFor(format), Transcoder::FormatName(format));
  }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
  return out;
}

inline int IndexOfFormat(Song::FileType type) {
  const auto choices = FormatChoices();
  for (size_t i = 0; i < choices.size(); ++i) {
    if (choices[i].first == type) {
      return static_cast<int>(i);
    }
  }
  return 0;
}

inline Song::FileType FormatAt(int index) {
  const auto choices = FormatChoices();
  if (index < 0 || index >= static_cast<int>(choices.size())) {
    return choices.front().first;
  }
  return choices[static_cast<size_t>(index)].first;
}

}  // namespace DevicePropertiesLabels

#endif
