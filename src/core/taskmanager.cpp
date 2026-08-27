#include "core/taskmanager.h"

#include <algorithm>

int TaskManager::StartTask(const std::string &name) {
  int id = 0;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    Task task;
    task.id = next_id_++;
    task.name = name;
    tasks_.push_back(task);
    id = task.id;
  }
  TasksChanged.Emit(id);
  return id;
}

void TaskManager::SetTaskProgress(int id, int progress, int progress_max) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Task &task : tasks_) {
      if (task.id == id) {
        task.progress = progress;
        task.progress_max = progress_max;
        break;
      }
    }
  }
  TasksChanged.Emit(id);
}

void TaskManager::SetTaskBlocksCollectionScans(int id) {
  bool found = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Task &task : tasks_) {
      if (task.id == id) {
        task.blocks_collection_scans = true;
        found = true;
        break;
      }
    }
  }
  if (!found) {
    return;
  }
  TasksChanged.Emit(id);
  PauseCollectionWatchers.Emit();
}

void TaskManager::SetTaskFinished(int id) {
  bool resume_collection_watchers = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const Task &task : tasks_) {
      if (task.id == id && task.blocks_collection_scans) {
        resume_collection_watchers = true;
        break;
      }
    }
    if (resume_collection_watchers) {
      for (const Task &task : tasks_) {
        if (task.id != id && task.blocks_collection_scans) {
          resume_collection_watchers = false;
          break;
        }
      }
    }
    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [id](const Task &task) { return task.id == id; }), tasks_.end());
  }
  TasksChanged.Emit(id);
  if (resume_collection_watchers) {
    ResumeCollectionWatchers.Emit();
  }
}

std::vector<TaskManager::Task> TaskManager::GetTasks() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return tasks_;
}
