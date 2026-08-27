#ifndef STRAWBERRY_COLLECTIONTASK_H
#define STRAWBERRY_COLLECTIONTASK_H

#include "core/taskmanager.h"

#include <string>

class CollectionTask {
 public:
  CollectionTask(TaskManager *task_manager, const std::string &name);
  ~CollectionTask();

  int id() const { return id_; }
  void SetProgress(int progress, int progress_max = 0);
  void Finish();

 private:
  TaskManager *task_manager_ = nullptr;
  int id_ = 0;
};

#endif
