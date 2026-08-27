#ifndef STRAWBERRY_COLLECTIONSCANGATES_H
#define STRAWBERRY_COLLECTIONSCANGATES_H

#include "core/taskmanager.h"

namespace CollectionScanGates {

inline bool AnyBlocksCollectionScans(const std::vector<TaskManager::Task> &tasks) {
  for (const TaskManager::Task &task : tasks) {
    if (task.blocks_collection_scans) {
      return true;
    }
  }
  return false;
}

inline bool ShouldSkipIncremental(const TaskManager *task_manager) {
  return task_manager && AnyBlocksCollectionScans(task_manager->GetTasks());
}

inline bool ShouldResumeWatchers(bool finishing_blocks, bool any_other_blocks) { return finishing_blocks && !any_other_blocks; }

}  // namespace CollectionScanGates

#endif  // STRAWBERRY_COLLECTIONSCANGATES_H
