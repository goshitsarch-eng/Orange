#ifndef STRAWBERRY_ENGINEDISCOVERER_H
#define STRAWBERRY_ENGINEDISCOVERER_H

#include "core/enginemetadata.h"
#include "core/song.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace EngineDiscoverer {

constexpr int kDiscoveryTimeoutS = 10;

constexpr int kResultOk = 0;
constexpr int kResultUriInvalid = 1;
constexpr int kResultError = 2;
constexpr int kResultTimeout = 3;
constexpr int kResultBusy = 4;
constexpr int kResultMissingPlugins = 5;

inline std::string SchemeOf(const std::string &url) {
  const auto colon = url.find(':');
  if (colon == std::string::npos || colon == 0) {
    return {};
  }
  std::string scheme = url.substr(0, colon);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return scheme;
}

// Qt GstEngine::Load / StartPreloading skip discovery for spotify:// (native plugin).
inline bool ShouldDiscover(const std::string &media_url) { return !media_url.empty() && SchemeOf(media_url) != "spotify"; }

inline std::string PlayUrl(const std::string &media_url, const std::string &stream_url) {
  return stream_url.empty() ? media_url : stream_url;
}

inline EngineMetadata FromAudioInfo(int samplerate, int bitdepth, int bitrate_bps) {
  EngineMetadata meta;
  meta.samplerate = samplerate;
  meta.bitdepth = bitdepth;
  meta.bitrate = bitrate_bps / 1000;
  return meta;
}

// Qt skips audio/mpeg because GStreamer uses it for both MP3 and AAC.
inline Song::FileType FiletypeFromCapsMimetype(const std::string &mimetype) {
  std::string mime = mimetype;
  std::transform(mime.begin(), mime.end(), mime.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (mime.empty() || mime == "audio/mpeg") {
    return Song::FileType::Unknown;
  }
  return Song::FiletypeByMimeType(mime);
}

inline Song::FileType FiletypeFromCodecDescription(const std::string &description) {
  if (description.empty()) {
    return Song::FileType::Unknown;
  }
  return Song::FiletypeByDescription(description);
}

inline Song::FileType ResolveFiletype(const std::string &mimetype, const std::string &codec_description) {
  const Song::FileType from_mime = FiletypeFromCapsMimetype(mimetype);
  if (from_mime != Song::FileType::Unknown) {
    return from_mime;
  }
  return FiletypeFromCodecDescription(codec_description);
}

inline EngineMetadata::Type MatchType(const std::string &discovered_url, const std::string &current_play_url,
                                      const std::string &next_play_url) {
  if (!discovered_url.empty() && discovered_url == current_play_url) {
    return EngineMetadata::Type::Current;
  }
  if (!discovered_url.empty() && discovered_url == next_play_url) {
    return EngineMetadata::Type::Next;
  }
  return EngineMetadata::Type::Any;
}

inline const char *ErrorMessage(int result) {
  switch (result) {
    case kResultUriInvalid:
      return "The URI is invalid";
    case kResultTimeout:
      return "The discovery timed-out";
    case kResultBusy:
      return "The discoverer was already discovering a file";
    case kResultMissingPlugins:
      return "Some plugins are missing for full discovery";
    case kResultError:
    default:
      return "An error happened and the GError is set";
  }
}

}  // namespace EngineDiscoverer

#endif  // STRAWBERRY_ENGINEDISCOVERER_H
