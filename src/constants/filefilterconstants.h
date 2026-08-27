#ifndef STRAWBERRY_FILEFILTERCONSTANTS_H
#define STRAWBERRY_FILEFILTERCONSTANTS_H

#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace FileFilterConstants {

// Combined audio + playlist filter matching Qt kFileFilter.
constexpr char kFileFilter[] =
    "*.wav *.flac *.wv *.ogg *.oga *.opus *.spx *.ape *.mpc "
    "*.mp2 *.mp3 *.m4a *.mp4 *.aac *.asf *.asx *.wma "
    "*.aif *.aiff *.mka *.tta *.dsf *.dsd *.dff *.webm "
    "*.cue *.m3u *.m3u8 *.pls *.xspf *.asxini "
    "*.ac3 *.dts "
    "*.mod *.s3m *.xm *.it "
    "*.spc *.vgm";

constexpr char kAudio[] =
    "*.mp3 *.mp2 *.flac *.ogg *.opus *.oga *.spx *.m4a *.mp4 *.aac *.wma *.asf "
    "*.wav *.aif *.aiff *.ape *.mpc *.wv *.tta *.dsf *.dsd *.dff *.mka *.webm "
    "*.ac3 *.dts *.mod *.s3m *.xm *.it *.spc *.vgm";

constexpr char kPlaylist[] = "*.m3u *.m3u8 *.pls *.xspf *.asx *.asxini *.wpl *.cue";

constexpr char kLoadImages[] = "*.png *.jpg *.jpeg *.bmp *.gif *.xpm *.pbm *.pgm *.ppm *.xbm *.webp";
constexpr char kSaveImages[] = "*.png *.jpg *.jpeg *.bmp *.xpm *.pbm *.ppm *.xbm *.webp";
constexpr char kAllFiles[] = "*";

inline std::vector<std::string> SplitGlobs(const char *globs) {
  std::vector<std::string> out;
  for (const std::string &part : StrUtils::Split(globs ? globs : "", ' ')) {
    if (!part.empty()) {
      out.push_back(part);
    }
  }
  return out;
}

inline bool ContainsExtension(const char *globs, const std::string &extension) {
  const std::string needle = "*." + StrUtils::ToLower(extension);
  for (const std::string &glob : SplitGlobs(globs)) {
    if (StrUtils::ToLower(glob) == needle) {
      return true;
    }
  }
  return false;
}

inline bool PathMatchesGlobs(const std::string &path, const char *globs) {
  const std::string lower = StrUtils::ToLower(path);
  for (const std::string &glob : SplitGlobs(globs)) {
    if (glob.size() > 1 && glob[0] == '*') {
      if (StrUtils::EndsWith(lower, StrUtils::ToLower(glob.substr(1)))) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace FileFilterConstants

#endif
