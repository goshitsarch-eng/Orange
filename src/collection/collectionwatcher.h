#ifndef STRAWBERRY_COLLECTIONWATCHER_H
#define STRAWBERRY_COLLECTIONWATCHER_H

#include "core/filesystemwatcherinotify.h"
#include "core/song.h"

#include <gio/gio.h>

#include <memory>
#include <string>
#include <vector>

class CollectionBackend;
class TagReader;
class TaskManager;

class CollectionWatcher {
 public:
  CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager);
  ~CollectionWatcher();

  void Scan();
  void ScanDirectory(int directory_id, const std::string &path, bool recursive);
  void StartWatching();
  void StopWatching();
  int last_added() const { return last_added_; }

 private:
  void ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added);
  void WatchPath(const std::string &path);

  CollectionBackend *backend_;
  TagReader *tagreader_;
  TaskManager *task_manager_;
  int last_added_ = 0;
  std::vector<GFileMonitor *> monitors_;
  std::unique_ptr<FileSystemWatcherInotify> inotify_watcher_;
};

#endif  // STRAWBERRY_COLLECTIONWATCHER_H
