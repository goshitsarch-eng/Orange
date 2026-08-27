#ifndef SONGLOADTYPEFIND_H
#define SONGLOADTYPEFIND_H

#include <cstddef>
#include <string>

namespace SongLoadTypefind {

// Qt SongLoader::DataReady magic threshold.
inline constexpr std::size_t kMagicSize = 512;

// Qt TypeFound: only text/plain and text/uri-list are treated as possible playlists.
inline bool MimeMightBePlaylist(const std::string &mime) { return mime == "text/plain" || mime == "text/uri-list"; }

enum class Decision {
  RawStream,
  Parse,
  Fail,
};

// Qt TypeFound + MagicReady + GST_STREAM_ERROR_TYPE_NOT_FOUND:
// audio/other mime → raw stream; text or unknown mime → parse if magic matches, else fail.
inline Decision Decide(const std::string &mime, bool magic_matched) {
  if (!mime.empty() && !MimeMightBePlaylist(mime)) {
    return Decision::RawStream;
  }
  return magic_matched ? Decision::Parse : Decision::Fail;
}

}  // namespace SongLoadTypefind

#endif  // SONGLOADTYPEFIND_H
