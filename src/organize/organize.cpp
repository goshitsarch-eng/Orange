#include "organize/organize.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <cstdio>
namespace {
std::string Safe(const std::string &value) {
  std::string result = value;
  for (char &c : result) if (c=='/' || c=='\\' || c==':' ) c = '_';
  return result;
}
}
std::string OrganizeFormat::GetFilenameForSong(const Song &song) const {
  std::string result = format_;
  auto replace = [&result](const std::string &token, const std::string &value) {
    result = StrUtils::Replace(result, token, Safe(value));
  };
  char track[8]; std::snprintf(track, sizeof(track), "%02d", song.track() > 0 ? song.track() : 0);
  replace("%albumartist", song.EffectiveAlbumartist());
  replace("%artist", song.artist());
  replace("%album", song.album());
  replace("%title", song.title());
  replace("%track", track);
  replace("%disc", std::to_string(song.disc() > 0 ? song.disc() : 1));
  replace("%genre", song.genre());
  replace("%year", song.year() > 0 ? std::to_string(song.year()) : "");
  result = StrUtils::Replace(result, "{", "");
  result = StrUtils::Replace(result, "}", "");
  return result;
}
bool Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move) {
  for (const Song &song : songs) {
    const std::string dest = FileUtils::Join(destination, format.GetFilenameForSong(song));
    g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
    const std::string src = FileUtils::PathFromUri(song.url());
    if (move) g_rename(src.c_str(), dest.c_str());
    else {
      const std::string data = FileUtils::ReadFile(src);
      FileUtils::WriteFile(dest, data);
    }
  }
  return true;
}
