#ifndef STRAWBERRY_ORGANIZETRANSCODE_H
#define STRAWBERRY_ORGANIZETRANSCODE_H

#include "core/musicstorage.h"
#include "core/song.h"
#include "device/devicedatabasebackend.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <string>
#include <vector>

class OrganizeTranscode {
 public:
  static MusicStorage::TranscodeMode FromDeviceMode(DeviceDatabaseBackend::TranscodeMode mode) {
    switch (mode) {
      case DeviceDatabaseBackend::TranscodeMode::Transcode_Never:
        return MusicStorage::TranscodeMode::Transcode_Never;
      case DeviceDatabaseBackend::TranscodeMode::Transcode_Always:
        return MusicStorage::TranscodeMode::Transcode_Always;
      case DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported:
        return MusicStorage::TranscodeMode::Transcode_Unsupported;
    }
    return MusicStorage::TranscodeMode::Transcode_Never;
  }

  static bool Contains(const std::vector<Song::FileType> &types, Song::FileType type) {
    return std::find(types.begin(), types.end(), type) != types.end();
  }

  static Song::FileType PickBestFormat(const std::vector<Song::FileType> &supported) {
    if (supported.empty()) {
      return Song::FileType::Unknown;
    }
    for (Song::FileType type : {Song::FileType::FLAC, Song::FileType::OggFlac, Song::FileType::WavPack}) {
      if (Contains(supported, type)) {
        return type;
      }
    }
    return supported.front();
  }

  static Song::FileType Check(Song::FileType original, MusicStorage::TranscodeMode mode, Song::FileType format,
                             const std::vector<Song::FileType> &supported) {
    if (original == Song::FileType::Stream) {
      return Song::FileType::Unknown;
    }
    switch (mode) {
      case MusicStorage::TranscodeMode::Transcode_Never:
        return Song::FileType::Unknown;
      case MusicStorage::TranscodeMode::Transcode_Always:
        return original == format ? Song::FileType::Unknown : format;
      case MusicStorage::TranscodeMode::Transcode_Unsupported:
        if (supported.empty() || Contains(supported, original)) {
          return Song::FileType::Unknown;
        }
        if (format != Song::FileType::Unknown) {
          return format;
        }
        return PickBestFormat(supported);
    }
    return Song::FileType::Unknown;
  }

  static Transcoder::Format FormatFromFileType(Song::FileType type) { return Transcoder::FormatFromFileType(type); }

  static bool CanTranscode(Song::FileType type) {
    switch (type) {
      case Song::FileType::MPEG:
      case Song::FileType::MP4:
      case Song::FileType::ALAC:
      case Song::FileType::OggVorbis:
      case Song::FileType::OggOpus:
      case Song::FileType::OggSpeex:
      case Song::FileType::WavPack:
      case Song::FileType::ASF:
      case Song::FileType::WAV:
      case Song::FileType::FLAC:
      case Song::FileType::OggFlac:
        return true;
      default:
        return false;
    }
  }

  static std::string ExtensionForFileType(Song::FileType type) { return Transcoder::Extension(FormatFromFileType(type)); }

  static std::string FiddleExtension(const std::string &path, const std::string &extension) {
    if (path.empty() || extension.empty()) {
      return path;
    }
    const std::string dir = FileUtils::DirName(path);
    std::string base = FileUtils::BaseName(path);
    const std::string current = FileUtils::Extension(base);
    if (!current.empty() && base.size() > current.size()) {
      base = base.substr(0, base.size() - current.size() - 1);
    }
    const std::string name = base + "." + extension;
    if (dir.empty() || dir == ".") {
      return name;
    }
    return FileUtils::Join(dir, name);
  }

  static std::vector<Song::FileType> SupportedForBackend(const std::string &backend) {
    if (backend == "gpod") {
      return {Song::FileType::MPEG, Song::FileType::MP4, Song::FileType::ALAC, Song::FileType::WAV};
    }
    if (backend == "mtp") {
      return {Song::FileType::MPEG, Song::FileType::MP4, Song::FileType::ASF, Song::FileType::WAV, Song::FileType::FLAC,
              Song::FileType::OggVorbis};
    }
    return {};
  }
};

#endif
