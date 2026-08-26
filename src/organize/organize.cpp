#include "organize/organize.h"

#include "core/filesystemmusicstorage.h"
#include "core/standardpaths.h"
#include "organize/organizepreview.h"
#include "organize/organizetranscode.h"
#include "transcoder/transcoder.h"
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
  const std::vector<OrganizePreview::Entry> entries =
      OrganizePreview::Compute(songs, format, options.transcode_mode, options.transcode_format, options.supported_filetypes);
  for (const OrganizePreview::Entry &entry : entries) {
    const Song &song = entry.song;
    const std::string src = FileUtils::PathFromUri(song.url());
    if (entry.relative_path.empty()) {
      errors.push_back({song.PrettyTitleWithArtist(), "Filename format produced an empty path"});
      continue;
    }
    std::string dest = FileUtils::Join(destination, entry.relative_path);
    const Song::FileType dest_type =
        OrganizeTranscode::Check(song.filetype(), options.transcode_mode, options.transcode_format, options.supported_filetypes);
    std::string temp;
    std::string copy_src = src;
    if (dest_type != Song::FileType::Unknown && OrganizeTranscode::CanTranscode(dest_type)) {
      temp = OrganizeTranscode::FiddleExtension(FileUtils::Join(StandardPaths::CacheDir(), "organize-" + FileUtils::BaseName(src)),
                                                OrganizeTranscode::ExtensionForFileType(dest_type));
      Transcoder transcoder;
      if (!transcoder.TranscodeFile(song, temp, OrganizeTranscode::FormatFromFileType(dest_type))) {
        errors.push_back({song.PrettyTitleWithArtist(), "Transcode failed"});
        continue;
      }
      copy_src = temp;
    }
    if (src.empty() || !FileUtils::Exists(copy_src)) {
      errors.push_back({song.PrettyTitleWithArtist(), "Source file is missing"});
      continue;
    }
    if (!options.overwrite && FileUtils::Exists(dest)) {
      errors.push_back({song.PrettyTitleWithArtist(), "Destination already exists"});
      if (!temp.empty()) {
        FileUtils::Remove(temp);
      }
      continue;
    }
    g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
    MusicStorage::CopyJob job;
    job.source = copy_src;
    job.destination = dest;
    job.metadata = song;
    job.overwrite = options.overwrite;
    job.remove_original = options.move && temp.empty();
    job.albumcover = options.albumcover;
    if (options.albumcover) {
      job.cover_source = CoverPathForSong(song);
      job.cover_dest = FileUtils::Join(FileUtils::DirName(dest), "cover.jpg");
    }
    std::string error_text;
    if (!storage.CopyToStorage(job, error_text)) {
      errors.push_back({song.PrettyTitleWithArtist(), error_text.empty() ? ("Could not write " + dest) : error_text});
    } else if (options.move && !temp.empty()) {
      FileUtils::Remove(src);
    }
    if (!temp.empty()) {
      FileUtils::Remove(temp);
    }
  }
  return errors;
}
