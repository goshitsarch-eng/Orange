#include "collection/collectiontask.h"

CollectionTask::CollectionTask(TaskManager *task_manager, const std::string &name) : task_manager_(task_manager) {
  if (task_manager_) {
    id_ = task_manager_->StartTask(name);
  }
}

CollectionTask::~CollectionTask() { Finish(); }

void CollectionTask::SetProgress(int progress, int progress_max) {
  if (task_manager_ && id_ > 0) {
    task_manager_->SetTaskProgress(id_, progress, progress_max);
  }
}

void CollectionTask::Finish() {
  if (task_manager_ && id_ > 0) {
    task_manager_->SetTaskFinished(id_);
    id_ = 0;
  }
}
