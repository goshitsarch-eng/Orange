#include "core/filesystemwatcherwin.h"

#include "core/filesystemwatcherwinpolicy.h"
#include "core/filesystemwatcherwinthread.h"

FileSystemWatcherWin::FileSystemWatcherWin() = default;

FileSystemWatcherWin::~FileSystemWatcherWin() { Clear(); }

FileSystemWatcherWinThread *FileSystemWatcherWin::ThreadForNewWatch() {
  for (auto &thread : threads_) {
    if (FileSystemWatcherWinPolicy::ThreadHasRoom(thread->WatchCount())) {
      return thread.get();
    }
  }
  threads_.push_back(std::make_unique<FileSystemWatcherWinThread>());
  FileSystemWatcherWinThread *thread = threads_.back().get();
  thread->PathChanged.Connect([this](const std::string &path) { PathChanged.Emit(path); });
  return thread;
}

void FileSystemWatcherWin::AddPath(const std::string &path) {
  const std::string key = FileSystemWatcherWinPolicy::PathKey(path);
  if (thread_from_path_.count(key)) {
    return;
  }
  FileSystemWatcherWinThread *thread = ThreadForNewWatch();
  if (thread->AddPath(path)) {
    thread_from_path_[key] = thread;
  }
}

void FileSystemWatcherWin::RemovePath(const std::string &path) {
  const std::string key = FileSystemWatcherWinPolicy::PathKey(path);
  auto it = thread_from_path_.find(key);
  if (it == thread_from_path_.end()) {
    return;
  }
  it->second->RemovePath(path);
  thread_from_path_.erase(it);
}

void FileSystemWatcherWin::Clear() {
  for (auto &thread : threads_) {
    thread->Stop();
  }
  threads_.clear();
  thread_from_path_.clear();
}
