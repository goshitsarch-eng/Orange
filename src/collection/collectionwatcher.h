#ifndef STRAWBERRY_COLLECTIONWATCHER_H
#define STRAWBERRY_COLLECTIONWATCHER_H

#include "core/song.h"

#include <string>
#include <vector>

class CollectionBackend;
class TagReader;
class TaskManager;

class CollectionWatcher {
 public:
  CollectionWatcher(CollectionBackend *backend, TagReader *tagreader, TaskManager *task_manager);

  void Scan();
  void ScanDirectory(int directory_id, const std::string &path, bool recursive);
  int last_added() const { return last_added_; }

 private:
  void ScanPath(int directory_id, const std::string &path, bool recursive, int task_id, int *added);

  CollectionBackend *backend_;
  TagReader *tagreader_;
  TaskManager *task_manager_;
  int last_added_ = 0;
};

#endif  // STRAWBERRY_COLLECTIONWATCHER_H
