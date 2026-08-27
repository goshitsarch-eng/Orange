#include "collection/collectionlibrary.h"

#include "collection/collectiondirectory.h"
#include "collection/collectionstats.h"
#include "constants/collectionsettings.h"
#include "core/logging.h"
#include "core/settings.h"
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

CollectionLibrary::~CollectionLibrary() {
  if (alive_) {
    *alive_ = false;
  }
}

void CollectionLibrary::Init() {
  watcher_->StartWatching();
  if (!task_manager_) {
    return;
  }
  const std::shared_ptr<bool> alive = alive_;
  task_manager_->PauseCollectionWatchers.Connect([this, alive]() {
    if (alive && *alive) {
      PauseWatcher();
    }
  });
  task_manager_->ResumeCollectionWatchers.Connect([this, alive]() {
    if (alive && *alive) {
      ResumeWatcher();
    }
  });
}

void CollectionLibrary::PauseWatcher() {
  if (watcher_) {
    watcher_->SetRescanPaused(true);
  }
}

void CollectionLibrary::ResumeWatcher() {
  if (watcher_) {
    watcher_->SetRescanPaused(false);
  }
}

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

void CollectionLibrary::CurrentSongChanged(const Song &song) {
  current_song_url_ = song.url();
  if (!pending_song_saves_.empty()) {
    SavePendingPlaycountsAndRatings();
  }
}

void CollectionLibrary::Stopped() {
  current_song_url_.clear();
  if (!pending_song_saves_.empty()) {
    SavePendingPlaycountsAndRatings();
  }
}

void CollectionLibrary::SongsPlaycountChanged(const SongList &songs, bool save_tags) {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  if (!save_tags && !settings.BoolValue(CollectionSettings::kSavePlayCounts, CollectionSettings::kDefaultSavePlayCounts)) {
    return;
  }
  if (!tagreader_) {
    return;
  }
  for (const Song &song : songs) {
    if (CollectionTagSave::ShouldDefer(song, current_song_url_)) {
      CollectionTagSave::Queue(&pending_song_saves_, song, true, false);
      continue;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    if (!path.empty()) {
      tagreader_->SavePlaycount(path, song.playcount());
    }
  }
}

void CollectionLibrary::SongsRatingChanged(const SongList &songs, bool save_tags) {
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  if (!save_tags && !settings.BoolValue(CollectionSettings::kSaveRatings, CollectionSettings::kDefaultSaveRatings)) {
    return;
  }
  if (!tagreader_) {
    return;
  }
  for (const Song &song : songs) {
    if (CollectionTagSave::ShouldDefer(song, current_song_url_)) {
      CollectionTagSave::Queue(&pending_song_saves_, song, false, true);
      continue;
    }
    const std::string path = FileUtils::PathFromUri(song.url());
    if (!path.empty()) {
      tagreader_->SaveRating(path, song.rating());
    }
  }
}

void CollectionLibrary::SavePendingPlaycountsAndRatings() {
  if (!tagreader_) {
    pending_song_saves_.clear();
    return;
  }
  const std::vector<std::string> ready = CollectionTagSave::ReadyToFlush(pending_song_saves_, current_song_url_);
  for (const std::string &url : ready) {
    const auto it = pending_song_saves_.find(url);
    if (it == pending_song_saves_.end()) {
      continue;
    }
    const CollectionTagSave::Pending &pending = it->second;
    const std::string path = FileUtils::PathFromUri(pending.song.url());
    if (!path.empty()) {
      if (pending.save_playcount) {
        tagreader_->SavePlaycount(path, pending.song.playcount());
      }
      if (pending.save_rating) {
        tagreader_->SaveRating(path, pending.song.rating());
      }
    }
    pending_song_saves_.erase(it);
  }
}
