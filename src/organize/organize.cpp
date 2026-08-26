#include "organize/organize.h"

#include "core/filesystemmusicstorage.h"
#include "utilities/fileutils.h"

#include <glib.h>
#include <glib/gstdio.h>

std::string Organize::CoverPathForSong(const Song &song) {
  for (const std::string &art : {song.art_manual(), song.art_automatic()}) {
    const std::string path = FileUtils::PathFromUri(art);
    if (!path.empty() && FileUtils::IsFile(path)) {
      return path;
    }
  }
  const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
  if (dir.empty()) {
    return {};
  }
  for (const char *name : {"cover.jpg", "cover.png", "folder.jpg", "front.jpg", "album.jpg"}) {
    const std::string path = FileUtils::Join(dir, name);
    if (FileUtils::IsFile(path)) {
      return path;
    }
  }
  return {};
}

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move) {
  Options options;
  options.move = move;
  return Copy(songs, destination, format, options);
}

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format,
                                            const Options &options) {
  std::vector<Error> errors;
  if (destination.empty()) {
    errors.push_back({"", "Destination folder is empty"});
    return errors;
  }
  FilesystemMusicStorage storage(destination);
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
    if (!options.overwrite && FileUtils::Exists(dest)) {
      errors.push_back({song.PrettyTitleWithArtist(), "Destination already exists"});
      continue;
    }
    g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
    MusicStorage::CopyJob job;
    job.source = src;
    job.destination = dest;
    job.metadata = song;
    job.overwrite = options.overwrite;
    job.remove_original = options.move;
    job.albumcover = options.albumcover;
    if (options.albumcover) {
      job.cover_source = CoverPathForSong(song);
      job.cover_dest = FileUtils::Join(FileUtils::DirName(dest), "cover.jpg");
    }
    std::string error_text;
    if (!storage.CopyToStorage(job, error_text)) {
      errors.push_back({song.PrettyTitleWithArtist(), error_text.empty() ? ("Could not write " + dest) : error_text});
    }
  }
  return errors;
}
