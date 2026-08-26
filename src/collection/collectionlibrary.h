#ifndef STRAWBERRY_COLLECTIONLIBRARY_H
#define STRAWBERRY_COLLECTIONLIBRARY_H

#include "collection/collectionbackend.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectionwatcher.h"
#include "core/signal.h"

#include <memory>
#include <string>

class Database;
class TagReader;
class TaskManager;

class CollectionLibrary {
 public:
  CollectionLibrary(Database *database, TaskManager *task_manager, TagReader *tagreader);

  CollectionBackend *backend() const { return backend_.get(); }
  CollectionWatcher *watcher() const { return watcher_.get(); }

  void Init();
  void IncrementalScan();
  void FullScan();
  void AbortScan();
  bool scanning() const;
  void Rescan(const SongList &songs);
  void RescanDirectory(int id);
  void AddDirectory(const std::string &path, bool subdirs = true);
  void RemoveDirectory(int id);
  SongList Songs(const std::string &filter = {}) const;
  SongList Songs(const CollectionFilterOptions &options) const;
  void SyncPlaycountAndRatingToFiles();
  void SyncPlaycountAndRatingToFilesAsync();

  Signal<> ScanFinished;

 private:
  Database *database_;
  TaskManager *task_manager_;
  TagReader *tagreader_;
  std::unique_ptr<CollectionBackend> backend_;
  std::unique_ptr<CollectionWatcher> watcher_;
};

#endif  // STRAWBERRY_COLLECTIONLIBRARY_H
