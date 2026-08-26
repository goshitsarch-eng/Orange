#include "core/deletefiles.h"

#include "utilities/fileutils.h"

DeleteFiles::DeleteFiles(TaskManager *task_manager, MusicStorage *storage, bool use_trash)
    : task_manager_(task_manager), storage_(storage), use_trash_(use_trash) {}

void DeleteFiles::Start(const SongList &songs) {
  errors_.clear();
  const int task = task_manager_ ? task_manager_->StartTask("Deleting files") : -1;
  int i = 0;
  for (const Song &song : songs) {
    MusicStorage::DeleteJob job;
    job.metadata = song;
    job.use_trash = use_trash_;
    bool ok = false;
    if (storage_) {
      ok = storage_->DeleteFromStorage(job);
    } else {
      const std::string path = FileUtils::PathFromUri(song.url());
      ok = !path.empty() && FileUtils::Remove(path);
    }
    if (!ok) {
      errors_.push_back(song);
    }
    if (task_manager_ && task >= 0) {
      task_manager_->SetTaskProgress(task, ++i, static_cast<int>(songs.size()));
    }
  }
  if (task_manager_ && task >= 0) {
    task_manager_->SetTaskFinished(task);
  }
  Finished.Emit(errors_);
}

void DeleteFiles::Start(const std::vector<std::string> &filenames) {
  SongList songs;
  for (const std::string &filename : filenames) {
    Song song;
    song.set_url(FileUtils::UriFromPath(filename));
    song.set_basefilename(FileUtils::BaseName(filename));
    songs.push_back(song);
  }
  Start(songs);
}
