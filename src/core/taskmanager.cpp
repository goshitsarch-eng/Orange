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

void TaskManager::SetTaskFinished(int id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.erase(std::remove_if(tasks_.begin(), tasks_.end(), [id](const Task &task) { return task.id == id; }), tasks_.end());
  }
  TasksChanged.Emit(id);
}

std::vector<TaskManager::Task> TaskManager::GetTasks() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return tasks_;
}
