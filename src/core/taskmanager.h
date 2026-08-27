#ifndef STRAWBERRY_TASKMANAGER_H
#define STRAWBERRY_TASKMANAGER_H

#include "core/signal.h"

#include <mutex>
#include <string>
#include <vector>

class TaskManager {
 public:
  struct Task {
    int id = 0;
    std::string name;
    int progress = 0;
    int progress_max = 0;
    bool blocks_collection_scans = false;
  };

  int StartTask(const std::string &name);
  void SetTaskProgress(int id, int progress, int progress_max = 0);
  void SetTaskBlocksCollectionScans(int id);
  void SetTaskFinished(int id);
  std::vector<Task> GetTasks() const;

  Signal<int> TasksChanged;
  Signal<> PauseCollectionWatchers;
  Signal<> ResumeCollectionWatchers;

 private:
  mutable std::mutex mutex_;
  std::vector<Task> tasks_;
  int next_id_ = 1;
};

#endif
