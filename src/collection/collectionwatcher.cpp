#include "collection/collectionwatcher.h"

#include "collection/collectionbackend.h"
#include "core/logging.h"
#include "core/settings.h"
#include "core/taskmanager.h"
#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

CollectionWatcher::CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager)
    : backend_(backend), tagreader_(tagreader), task_manager_(task_manager) {}

CollectionWatcher::~CollectionWatcher() { StopWatching(); }

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
    inotify_watcher_->PathChanged.Connect([this](const std::string &) { Scan(); });
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
                       static_cast<CollectionWatcher *>(data)->Scan();
                     }
                   }),
                   this);
  monitors_.push_back(monitor);
}

void CollectionWatcher::StartWatching() {
  StopWatching();
  Settings settings;
  settings.BeginGroup("Collection");
  if (!settings.BoolValue("monitor", true) || !backend_) {
    return;
  }
  for (const CollectionDirectory &directory : backend_->Directories()) {
    WatchPath(directory.path);
  }
}

void CollectionWatcher::Scan() {
  last_added_ = 0;
  if (!backend_) {
    return;
  }
  const int task_id = task_manager_ ? task_manager_->StartTask("Scanning collection") : 0;
  for (const CollectionDirectory &directory : backend_->Directories()) {
    int added = 0;
    ScanPath(directory.id, directory.path, directory.subdirs, task_id, &added);
    last_added_ += added;
  }
  if (task_manager_ && task_id) {
    task_manager_->SetTaskFinished(task_id);
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
