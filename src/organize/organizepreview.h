#ifndef STRAWBERRY_ORGANIZEPREVIEW_H
#define STRAWBERRY_ORGANIZEPREVIEW_H

#include "core/song.h"
#include "organize/organizeformat.h"
#include "organize/organizetranscode.h"
#include "utilities/fileutils.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace OrganizePreview {

struct Entry {
  Song song;
  std::string relative_path;
  bool unique_filename = false;
  bool ok = true;
};

inline std::vector<std::pair<std::string, std::string>> AfterCopyChoices() {
  return {{"keep", "Keep the original files"}, {"delete", "Delete the original files"}};
}

inline bool DeleteOriginals(const std::string &id) { return id == "delete"; }

inline const char *AfterCopyId(bool move) { return move ? "delete" : "keep"; }

// Qt OrganizeDialog::SetCopy(true) for copy-to-device always starts on "keep originals".
inline bool IsDeviceCopy(const std::string &device_id, bool show_eject) { return !device_id.empty() || show_eject; }

inline bool DeviceCopyKeepsOriginals() { return true; }

inline bool InitialMove(bool request_move, bool device_copy, bool persisted_move) {
  if (device_copy) {
    return request_move;
  }
  return request_move || persisted_move;
}

inline bool ShouldPersistMove(bool device_copy) { return !device_copy; }

inline std::string InsertBeforeExtension(const std::string &path, const std::string &insert) {
  const std::string ext = FileUtils::Extension(path);
  if (ext.empty()) {
    return path + insert;
  }
  if (path.size() <= ext.size() + 1) {
    return path + insert;
  }
  return path.substr(0, path.size() - ext.size() - 1) + insert + "." + ext;
}

inline std::string Disambiguate(const std::string &path, std::map<std::string, int> *counts) {
  if (!counts) {
    return path;
  }
  int &seen = (*counts)[path];
  ++seen;
  if (seen == 1) {
    return path;
  }
  return InsertBeforeExtension(path, "(" + std::to_string(seen) + ")");
}

inline std::vector<Entry> Compute(const SongList &songs, const OrganizeFormat &format, const std::string &extension = {}) {
  std::vector<Entry> entries;
  std::map<std::string, int> counts;
  for (const Song &song : songs) {
    OrganizeFormat::GetFilenameResult result = format.GetFilenameForSongResult(song, extension);
    Entry entry;
    entry.song = song;
    entry.unique_filename = result.unique_filename;
    if (result.path.empty()) {
      entry.ok = false;
      entries.push_back(entry);
      continue;
    }
    if (result.unique_filename) {
      entry.relative_path = Disambiguate(result.path, &counts);
    } else {
      entry.relative_path = result.path;
      entry.ok = false;
    }
    entries.push_back(entry);
  }
  return entries;
}

inline std::vector<Entry> Compute(const SongList &songs, const OrganizeFormat &format, MusicStorage::TranscodeMode mode,
                                  Song::FileType transcode_format, const std::vector<Song::FileType> &supported) {
  std::vector<Entry> entries;
  std::map<std::string, int> counts;
  for (const Song &song : songs) {
    const Song::FileType dest_type = OrganizeTranscode::Check(song.filetype(), mode, transcode_format, supported);
    const std::string extension =
        dest_type != Song::FileType::Unknown && OrganizeTranscode::CanTranscode(dest_type) ? OrganizeTranscode::ExtensionForFileType(dest_type)
                                                                                          : std::string();
    OrganizeFormat::GetFilenameResult result = format.GetFilenameForSongResult(song, extension);
    Entry entry;
    entry.song = song;
    entry.unique_filename = result.unique_filename;
    if (result.path.empty()) {
      entry.ok = false;
      entries.push_back(entry);
      continue;
    }
    if (result.unique_filename) {
      entry.relative_path = Disambiguate(result.path, &counts);
    } else {
      entry.relative_path = result.path;
      entry.ok = false;
    }
    entries.push_back(entry);
  }
  return entries;
}

inline int64_t TotalBytes(const SongList &songs) {
  int64_t total = 0;
  for (const Song &song : songs) {
    const int64_t size = song.filesize();
    if (size > 0) {
      total += size;
    }
  }
  return total;
}

inline bool AnyEmptyPath(const std::vector<Entry> &entries) {
  for (const Entry &entry : entries) {
    if (entry.relative_path.empty()) {
      return true;
    }
  }
  return false;
}

inline bool CanProceed(const std::vector<Entry> &entries) {
  if (entries.empty() || AnyEmptyPath(entries)) {
    return false;
  }
  for (const Entry &entry : entries) {
    if (!entry.ok) {
      return false;
    }
  }
  return true;
}

inline bool FitsOnDevice(int64_t additional, int64_t used, int64_t total) {
  if (total <= 0) {
    return true;
  }
  return additional <= total - used;
}

// Qt OrganizeDialog::UpdatePreviews: OK stays off without a usable destination.
inline bool HasDestination(const std::string &destination) { return !destination.empty(); }

// Qt hides groupbox_naming / groupbox_preview unless storage->LocalPath() is set.
inline bool HasLocalDestination(const std::string &destination) { return !destination.empty(); }

inline bool ShowsNamingPreview(bool has_local_destination) { return has_local_destination; }

// Organize-to-folder always names files. Device copy only does when a mount path is set.
inline bool ShowsNamingPreview(const std::string &destination, const std::string &device_id) {
  return ShowsNamingPreview(device_id.empty() || HasLocalDestination(destination));
}

inline bool FormatRequired(bool has_local_destination) { return has_local_destination; }

inline bool FormatValidForRun(bool format_valid, bool has_local_destination) {
  return !FormatRequired(has_local_destination) || format_valid;
}

// Copy-to-device pre-fills a mount path or device id; Qt destination combo stays on that device.
inline bool LocksDestination(const std::string &destination, const std::string &device_id) {
  return !destination.empty() || !device_id.empty();
}

inline bool CanRun(bool format_valid, const std::string &destination, const std::vector<Entry> &entries, int64_t additional_bytes,
                   int64_t used_bytes, int64_t total_bytes, bool has_local_destination = true, bool has_songs = true) {
  if (!FormatValidForRun(format_valid, has_local_destination) || !FitsOnDevice(additional_bytes, used_bytes, total_bytes)) {
    return false;
  }
  if (!has_local_destination) {
    return has_songs;
  }
  return HasDestination(destination) && CanProceed(entries);
}

inline const char *PreviewIconName(const Entry &entry) { return entry.ok ? "dialog-ok-apply-symbolic" : "dialog-warning-symbolic"; }

}  // namespace OrganizePreview

#endif
