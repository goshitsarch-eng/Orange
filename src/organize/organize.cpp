#include "organize/organize.h"

#include "utilities/fileutils.h"

#include <glib.h>
#include <glib/gstdio.h>

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move) {
  std::vector<Error> errors;
  if (destination.empty()) {
    errors.push_back({"", "Destination folder is empty"});
    return errors;
  }
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    const std::string relative = format.GetFilenameForSong(song);
    if (relative.empty()) {
      errors.push_back({song.PrettyTitleWithArtist(), "Filename format produced an empty path"});
      continue;
    }
    const std::string dest = FileUtils::Join(destination, relative);
    if (src.empty() || !FileUtils::Exists(src)) {
      errors.push_back({song.PrettyTitleWithArtist(), "Source file is missing"});
      continue;
    }
    g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
    bool ok = false;
    if (move) {
      ok = g_rename(src.c_str(), dest.c_str()) == 0;
      if (!ok) {
        ok = FileUtils::CopyFile(src, dest) && FileUtils::Remove(src);
      }
    } else {
      ok = FileUtils::CopyFile(src, dest);
    }
    if (!ok) {
      errors.push_back({song.PrettyTitleWithArtist(), "Could not write " + dest});
    }
  }
  return errors;
}
