#ifndef STRAWBERRY_COLLECTIONWATCHER_H
#define STRAWBERRY_COLLECTIONWATCHER_H

#include "collection/collectiondirectory.h"
#include "core/filesystemwatcherinotify.h"
#include "core/signal.h"
#include "core/song.h"

#include <gio/gio.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CollectionBackend;
class TagReader;
class TaskManager;

class CollectionWatcher {
 public:
  enum class ScanType { Incremental, Full };

  CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager);
  ~CollectionWatcher();

  void Scan();
  void Scan(ScanType type);
  void ScanDirectory(int directory_id, const std::string &path, bool recursive);
  void Abort();
  bool scanning() const { return scanning_; }
  void StartWatching();
  void StopWatching();
  int last_added() const { return last_added_; }

  static bool NeedsRescan(const Song &existing, int64_t mtime, int64_t filesize) {
    if (!existing.is_valid() || existing.unavailable()) {
      return true;
    }
    return !(mtime > 0 && existing.mtime() == mtime && (filesize < 0 || existing.filesize() == filesize));
  }

  Signal<> ScanFinished;

 private:
  struct ExistingInfo {
    int64_t mtime = -1;
    int64_t filesize = -1;
    bool unavailable = false;
    bool valid = false;
  };

  struct ScanJob {
    CollectionWatcher *watcher = nullptr;
    std::shared_ptr<std::atomic<bool>> alive;
    ScanType type = ScanType::Incremental;
    std::vector<CollectionDirectory> directories;
    std::map<std::string, ExistingInfo> existing;
    SongList songs;
    std::vector<std::string> seen_urls;
    bool aborted = false;
    int added = 0;
  };

  void ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added);
  void WatchPath(const std::string &path);
  void StartAsyncScan(ScanType type);
  static gpointer ScanThread(gpointer data);
  static gboolean ApplyScanJob(gpointer data);
  void CollectDirectory(ScanJob *job, const CollectionDirectory &directory);

  CollectionBackend *backend_;
  TagReader *tagreader_;
  TaskManager *task_manager_;
  int last_added_ = 0;
  std::atomic<bool> abort_{false};
  std::atomic<bool> scanning_{false};
  std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
  std::vector<GFileMonitor *> monitors_;
  std::unique_ptr<FileSystemWatcherInotify> inotify_watcher_;
};

#endif  // STRAWBERRY_COLLECTIONWATCHER_H
