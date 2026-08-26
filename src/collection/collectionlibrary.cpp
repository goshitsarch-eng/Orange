#include "collection/collectionlibrary.h"

#include "collection/collectiondirectory.h"
#include "collection/collectionstats.h"
#include "core/logging.h"
#include "core/taskmanager.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <thread>

CollectionLibrary::CollectionLibrary(Database *database, TaskManager *task_manager, TagReader *tagreader)
    : database_(database),
      task_manager_(task_manager),
      tagreader_(tagreader),
      backend_(std::make_unique<CollectionBackend>(database)),
      watcher_(std::make_unique<CollectionWatcher>(backend_.get(), tagreader, task_manager)) {
  watcher_->ScanFinished.Connect([this]() { ScanFinished.Emit(); });
}

void CollectionLibrary::Init() { watcher_->StartWatching(); }

void CollectionLibrary::IncrementalScan() { watcher_->Scan(CollectionWatcher::ScanType::Incremental); }

void CollectionLibrary::FullScan() { watcher_->Scan(CollectionWatcher::ScanType::Full); }

void CollectionLibrary::AbortScan() {
  if (watcher_) {
    watcher_->Abort();
  }
}

bool CollectionLibrary::scanning() const { return watcher_ && watcher_->scanning(); }

void CollectionLibrary::Rescan(const SongList &songs) {
  for (const Song &song : songs) {
    const std::string path = FileUtils::PathFromUri(song.url());
    if (path.empty() || !FileUtils::Exists(path) || !tagreader_) {
      continue;
    }
    Song updated = tagreader_->ReadFile(path);
    if (!updated.is_valid()) {
      continue;
    }
    if (song.id() > 0) {
      updated.set_id(song.id());
    }
    backend_->AddOrUpdateSong(updated);
  }
  ScanFinished.Emit();
}

void CollectionLibrary::RescanDirectory(int id) {
  for (const CollectionDirectory &directory : backend_->Directories()) {
    if (directory.id == id) {
      watcher_->ScanDirectory(id, directory.path, directory.subdirs);
      return;
    }
  }
}

void CollectionLibrary::AddDirectory(const std::string &path, bool subdirs) {
  const int id = backend_->AddDirectory(path, subdirs);
  if (id >= 0) {
    watcher_->ScanDirectory(id, path, subdirs);
    watcher_->StartWatching();
  }
  ScanFinished.Emit();
}

void CollectionLibrary::RemoveDirectory(int id) { backend_->RemoveDirectory(id); }

SongList CollectionLibrary::Songs(const std::string &filter) const { return backend_->Songs(filter); }

SongList CollectionLibrary::Songs(const CollectionFilterOptions &options) const { return backend_->Songs(options); }

void CollectionLibrary::SyncPlaycountAndRatingToFiles() {
  if (!tagreader_ || !backend_ || !task_manager_) {
    return;
  }
  const int task_id = task_manager_->StartTask(CollectionStats::TaskName());
  const SongList songs = backend_->Songs();
  int i = 0;
  for (const Song &song : songs) {
    ++i;
    if (CollectionStats::ShouldWriteStatistics(song)) {
      const std::string path = FileUtils::PathFromUri(song.url());
      tagreader_->SavePlaycount(path, song.playcount());
      tagreader_->SaveRating(path, song.rating());
    }
    task_manager_->SetTaskProgress(task_id, i, static_cast<int>(songs.size()));
  }
  task_manager_->SetTaskFinished(task_id);
}

void CollectionLibrary::SyncPlaycountAndRatingToFilesAsync() {
  std::thread([this]() { SyncPlaycountAndRatingToFiles(); }).detach();
}
