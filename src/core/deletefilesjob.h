#ifndef STRAWBERRY_DELETEFILESJOB_H
#define STRAWBERRY_DELETEFILESJOB_H

#include "core/song.h"
#include "organize/organize.h"
#include "utilities/fileutils.h"

#include <set>
#include <string>
#include <vector>

namespace DeleteFilesJob {

inline constexpr int kBatchSize = 50;

inline const char *TaskName() { return "Deleting files"; }

inline bool ShouldProcessBatch(bool cancelled) { return !cancelled; }

inline bool ShouldFinish(int next, int total, bool cancelled) { return cancelled || next >= total; }

inline bool ShouldScheduleNext(int next, int total, bool cancelled, bool async) {
  return async && !cancelled && next < total;
}

inline int Progress(int complete) { return complete; }

inline int ProgressMax(int total) { return total; }

inline bool DeletePath(const std::string &path, bool use_trash) {
  if (path.empty() || !FileUtils::Exists(path)) {
    return false;
  }
  if (use_trash && FileUtils::MoveToTrash(path)) {
    return true;
  }
  return FileUtils::Remove(path);
}

inline std::vector<Organize::Error> ToOrganizeErrors(const SongList &songs) {
  std::vector<Organize::Error> errors;
  errors.reserve(songs.size());
  for (const Song &song : songs) {
    Organize::Error error;
    error.song = song.PrettyTitleWithArtist();
    if (error.song.empty()) {
      error.song = song.basefilename();
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    error.message = path.empty() ? "Could not delete file" : ("Could not delete " + path);
    errors.push_back(error);
  }
  return errors;
}

inline std::vector<int> RowsToRemove(const SongList &playlist, const SongList &requested, const SongList &errors) {
  std::set<std::string> failed;
  for (const Song &song : errors) {
    failed.insert(song.url());
  }
  std::set<std::string> wanted;
  for (const Song &song : requested) {
    wanted.insert(song.url());
  }
  std::vector<int> rows;
  for (size_t i = 0; i < playlist.size(); ++i) {
    if (wanted.count(playlist[i].url()) && !failed.count(playlist[i].url())) {
      rows.push_back(static_cast<int>(i));
    }
  }
  return rows;
}

}  // namespace DeleteFilesJob

#endif  // STRAWBERRY_DELETEFILESJOB_H
