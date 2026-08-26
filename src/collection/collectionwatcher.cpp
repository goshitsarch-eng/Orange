#include "collection/collectionwatcher.h"

#include "collection/collectionbackend.h"
#include "constants/collectionsettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "core/taskmanager.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

#include <set>

CollectionWatcher::CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager)
    : backend_(backend), tagreader_(tagreader), task_manager_(task_manager) {}

CollectionWatcher::~CollectionWatcher() {
  Abort();
  if (alive_) {
    *alive_ = false;
  }
  StopWatching();
}

void CollectionWatcher::Abort() { abort_ = true; }

void CollectionWatcher::Scan() { Scan(ScanType::Incremental); }

void CollectionWatcher::Scan(ScanType type) {
  if (scanning_) {
    return;
  }
  StartAsyncScan(type);
}

void CollectionWatcher::StartAsyncScan(ScanType type) {
  if (!backend_ || scanning_) {
    return;
  }
  abort_ = false;
  scanning_ = true;
  auto *job = new ScanJob;
  job->watcher = this;
  job->alive = alive_;
  job->type = type;
  job->directories = backend_->Directories();
  if (type == ScanType::Incremental) {
    for (const Song &song : backend_->Songs()) {
      ExistingInfo info;
      info.mtime = song.mtime();
      info.filesize = song.filesize();
      info.unavailable = song.unavailable();
      info.valid = song.is_valid();
      job->existing[song.url()] = info;
    }
  }
  g_thread_unref(g_thread_new("collection-scan", ScanThread, job));
}

gpointer CollectionWatcher::ScanThread(gpointer data) {
  auto *job = static_cast<ScanJob *>(data);
  if (job->watcher) {
    for (const CollectionDirectory &directory : job->directories) {
      if (job->watcher->abort_) {
        job->aborted = true;
        break;
      }
      job->watcher->CollectDirectory(job, directory);
    }
  }
  g_idle_add(ApplyScanJob, job);
  return nullptr;
}

void CollectionWatcher::CollectDirectory(ScanJob *job, const CollectionDirectory &directory) {
  const std::vector<std::string> entries =
      directory.subdirs ? FileUtils::ListDirectoryRecursive(directory.path) : FileUtils::ListDirectory(directory.path);
  for (const std::string &entry : entries) {
    if (abort_) {
      job->aborted = true;
      return;
    }
    if (FileUtils::IsDirectory(entry) || !Song::IsAudioFile(entry)) {
      continue;
    }
    const std::string url = FileUtils::UriFromPath(entry);
    job->seen_urls.push_back(url);
    Song existing;
    auto it = job->existing.find(url);
    if (it != job->existing.end()) {
      existing.set_valid(it->second.valid);
      existing.set_mtime(it->second.mtime);
      existing.set_filesize(it->second.filesize);
      existing.set_unavailable(it->second.unavailable);
    }
    if (job->type == ScanType::Incremental && !NeedsRescan(existing, FileUtils::FileMtime(entry), FileUtils::FileSize(entry))) {
      continue;
    }
    if (!tagreader_) {
      continue;
    }
    Song song = tagreader_->ReadFile(entry);
    song.set_source(Song::Source::Collection);
    song.set_directory_id(directory.id);
    if (song.url().empty()) {
      continue;
    }
    job->songs.push_back(song);
    ++job->added;
    if (task_manager_ && (job->added % 25) == 0) {
      // Progress is applied on the main thread after the walk finishes.
    }
  }
}

gboolean CollectionWatcher::ApplyScanJob(gpointer data) {
  std::unique_ptr<ScanJob> job(static_cast<ScanJob *>(data));
  if (!job->alive || !*job->alive || !job->watcher) {
    return G_SOURCE_REMOVE;
  }
  CollectionWatcher *self = job->watcher;
  const int task_id = self->task_manager_ ? self->task_manager_->StartTask(job->type == ScanType::Full ? "Full collection scan" : "Scanning collection") : 0;
  self->last_added_ = 0;
  if (self->backend_) {
    for (const Song &song : job->songs) {
      self->backend_->AddOrUpdateSong(song);
      ++self->last_added_;
      if (self->task_manager_ && task_id && (self->last_added_ % 25) == 0) {
        self->task_manager_->SetTaskProgress(task_id, self->last_added_);
      }
    }
    Settings settings;
    settings.BeginGroup(CollectionSettings::kSettingsGroup);
    const bool mark_unavailable =
        job->type == ScanType::Full && !job->aborted &&
        settings.BoolValue(CollectionSettings::kMarkSongsUnavailable, CollectionSettings::kDefaultMarkSongsUnavailable);
    if (mark_unavailable) {
      std::set<int> directories;
      for (const CollectionDirectory &directory : job->directories) {
        directories.insert(directory.id);
      }
      for (int directory_id : directories) {
        self->backend_->MarkMissingUnavailable(directory_id, job->seen_urls);
      }
    }
  }
  if (self->task_manager_ && task_id) {
    self->task_manager_->SetTaskFinished(task_id);
  }
  self->scanning_ = false;
  self->ScanFinished.Emit();
  return G_SOURCE_REMOVE;
}

void CollectionWatcher::StopWatching() {
  for (GFileMonitor *monitor : monitors_) {
    g_file_monitor_cancel(monitor);
    g_object_unref(monitor);
  }
  monitors_.clear();
  if (inotify_watcher_) {
    inotify_watcher_->Clear();
  }
}

void CollectionWatcher::WatchPath(const std::string &path) {
  if (!inotify_watcher_) {
    inotify_watcher_ = std::make_unique<FileSystemWatcherInotify>();
    inotify_watcher_->PathChanged.Connect([this](const std::string &) { Scan(ScanType::Incremental); });
  }
  inotify_watcher_->AddPath(path);
  GFile *file = g_file_new_for_path(path.c_str());
  GFileMonitor *monitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
  g_object_unref(file);
  if (!monitor) {
    return;
  }
  g_signal_connect(monitor, "changed", G_CALLBACK(+[](GFileMonitor *, GFile *, GFile *, GFileMonitorEvent event, gpointer data) {
                     if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT || event == G_FILE_MONITOR_EVENT_CREATED ||
                         event == G_FILE_MONITOR_EVENT_DELETED) {
                       static_cast<CollectionWatcher *>(data)->Scan(ScanType::Incremental);
                     }
                   }),
                   this);
  monitors_.push_back(monitor);
}

void CollectionWatcher::StartWatching() {
  StopWatching();
  Settings settings;
  settings.BeginGroup(CollectionSettings::kSettingsGroup);
  if (!settings.BoolValue(CollectionSettings::kMonitor, CollectionSettings::kDefaultMonitor) || !backend_) {
    return;
  }
  for (const CollectionDirectory &directory : backend_->Directories()) {
    WatchPath(directory.path);
  }
}

void CollectionWatcher::ScanDirectory(int directory_id, const std::string &path, bool recursive) {
  int added = 0;
  const int task_id = task_manager_ ? task_manager_->StartTask("Scanning " + path) : 0;
  ScanPath(directory_id, path, recursive, task_id, &added);
  last_added_ = added;
  if (task_manager_ && task_id) {
    task_manager_->SetTaskFinished(task_id);
  }
}

void CollectionWatcher::ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added) {
  for (const std::string &entry : FileUtils::ListDirectory(path)) {
    if (abort_) {
      return;
    }
    if (FileUtils::IsDirectory(entry)) {
      if (recursive) {
        ScanPath(directory_id, entry, true, task_id, added);
      }
      continue;
    }
    if (!Song::IsAudioFile(entry)) {
      continue;
    }
    Song song = tagreader_->ReadFile(entry);
    song.set_source(Song::Source::Collection);
    song.set_directory_id(directory_id);
    if (song.url().empty()) {
      continue;
    }
    backend_->AddOrUpdateSong(song);
    ++(*added);
    if (task_manager_ && task_id && (*added % 25) == 0) {
      task_manager_->SetTaskProgress(task_id, *added);
    }
  }
}
