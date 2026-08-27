#ifndef SONGLOADREMOTE_H
#define SONGLOADREMOTE_H

#include "core/commandlineurl.h"
#include "core/song.h"
#include "core/songloadurl.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <string>

namespace SongLoadRemote {

enum class Kind {
  RawStream,
  Probe,
};

inline std::string PathWithoutQuery(const std::string &url) {
  std::string path = url;
  const auto q = path.find('?');
  if (q != std::string::npos) path = path.substr(0, q);
  const auto hash = path.find('#');
  if (hash != std::string::npos) path = path.substr(0, hash);
  return path;
}

inline std::string Extension(const std::string &url) { return StrUtils::ToLower(FileUtils::Extension(PathWithoutQuery(url))); }

inline bool LooksLikePlaylist(const std::string &url) {
  const std::string ext = Extension(url);
  return ext == "m3u" || ext == "m3u8" || ext == "pls" || ext == "xspf" || ext == "asx" || ext == "asxini" || ext == "wpl" || ext == "cue";
}

inline bool LooksLikeAudio(const std::string &url) { return Song::IsAudioFile(PathWithoutQuery(url)); }

// Qt Load() streams only sRawUriSchemes or a registered UrlHandler.
// Known audio extensions skip a remote probe so playback starts without a fetch.
// Playlist extensions and unknown remotes need LoadRemote (HTTP + magic).
inline Kind Classify(const std::string &url) {
  if (SongLoadUrl::IsRawStreamScheme(CommandlineUrl::Scheme(url))) return Kind::RawStream;
  if (LooksLikePlaylist(url)) return Kind::Probe;
  if (LooksLikeAudio(url)) return Kind::RawStream;
  return Kind::Probe;
}

inline bool ShouldAddAsRawStream(const std::string &url) { return Classify(url) == Kind::RawStream; }

}  // namespace SongLoadRemote

#endif  // SONGLOADREMOTE_H
