#include "core/deletefiles.h"

#include "core/deletefilesjob.h"
#include "utilities/fileutils.h"

#include <glib.h>

namespace {

gboolean DeleteFilesProcessIdle(gpointer data) {
  static_cast<DeleteFiles *>(data)->IdleTick();
  return G_SOURCE_REMOVE;
}

}  // namespace

DeleteFiles::DeleteFiles(TaskManager *task_manager, MusicStorage *storage, bool use_trash)
    : task_manager_(task_manager), storage_(storage), use_trash_(use_trash) {}

DeleteFiles::~DeleteFiles() {
  if (idle_id_) {
    g_source_remove(idle_id_);
    idle_id_ = 0;
  }
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
}

void DeleteFiles::Start(const SongList &songs) {
  Begin(songs);
  async_ = false;
  if (task_manager_) {
    task_id_ = task_manager_->StartTask(DeleteFilesJob::TaskName());
    task_manager_->SetTaskBlocksCollectionScans(task_id_);
  }
  while (!finished_) {
    ProcessSome();
  }
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

void DeleteFiles::Begin(const SongList &songs) {
  songs_ = songs;
  errors_.clear();
  next_ = 0;
  cancelled_ = false;
  finished_ = false;
  async_ = false;
  started_storage_ = false;
}

void DeleteFiles::StartAsync(const SongList &songs) {
  Begin(songs);
  async_ = true;
  if (task_manager_) {
    task_id_ = task_manager_->StartTask(DeleteFilesJob::TaskName());
    task_manager_->SetTaskBlocksCollectionScans(task_id_);
  }
  ScheduleIdle();
}

void DeleteFiles::Cancel() { cancelled_ = true; }

void DeleteFiles::IdleTick() {
  idle_id_ = 0;
  ProcessSome();
}

void DeleteFiles::ScheduleIdle() {
  if (idle_id_) {
    return;
  }
  idle_id_ = g_idle_add(DeleteFilesProcessIdle, this);
}

void DeleteFiles::Complete() {
  if (started_storage_ && storage_) {
    std::string error_text;
    storage_->FinishDelete(errors_.empty(), error_text);
  }
  if (task_id_ > 0 && task_manager_) {
    task_manager_->SetTaskFinished(task_id_);
    task_id_ = 0;
  }
  finished_ = true;
  Finished.Emit(errors_);
}

void DeleteFiles::ProcessSome() {
  if (finished_) {
    return;
  }
  const int total = static_cast<int>(songs_.size());
  if (!started_storage_ && storage_) {
    storage_->StartDelete();
    started_storage_ = true;
  }
  if (!DeleteFilesJob::ShouldProcessBatch(cancelled_)) {
    if (DeleteFilesJob::ShouldFinish(next_, total, cancelled_)) {
      Complete();
    }
    return;
  }
  int processed = 0;
  while (processed < DeleteFilesJob::kBatchSize && next_ < total) {
    ProcessOne(songs_[static_cast<size_t>(next_)]);
    ++next_;
    ++processed;
  }
  if (task_manager_ && task_id_ > 0) {
    task_manager_->SetTaskProgress(task_id_, DeleteFilesJob::Progress(next_), DeleteFilesJob::ProgressMax(total));
  }
  if (DeleteFilesJob::ShouldFinish(next_, total, cancelled_)) {
    Complete();
    return;
  }
  if (DeleteFilesJob::ShouldScheduleNext(next_, total, cancelled_, async_)) {
    ScheduleIdle();
  }
}

void DeleteFiles::ProcessOne(const Song &song) {
  MusicStorage::DeleteJob job;
  job.metadata = song;
  job.use_trash = use_trash_;
  bool ok = false;
  if (storage_) {
    ok = storage_->DeleteFromStorage(job);
  } else {
    ok = DeleteFilesJob::DeletePath(FileUtils::PathFromUri(song.url()), use_trash_);
  }
  if (!ok) {
    errors_.push_back(song);
  }
}
