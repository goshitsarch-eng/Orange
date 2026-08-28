#ifndef STRAWBERRY_FILESYSTEMWATCHERWINTHREAD_H
#define STRAWBERRY_FILESYSTEMWATCHERWINTHREAD_H

#include "core/signal.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class FileSystemWatcherWinThread {
 public:
  FileSystemWatcherWinThread();
  ~FileSystemWatcherWinThread();

  bool AddPath(const std::string &path);
  bool RemovePath(const std::string &path);
  bool IsEmpty() const;
  int WatchCount() const;
  void Stop();

  Signal<std::string> PathChanged;
  Signal<std::string> WatchDropped;

 private:
  void Run();
  void SchedulePathChanged(const std::string &path, bool dropped);
#ifdef _WIN32
  void ClosePending();
  static DWORD WINAPI ThreadProc(LPVOID self);
#endif

  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

#ifdef _WIN32
  mutable std::mutex mutex_;
  HANDLE wakeup_ = nullptr;
  std::map<std::string, HANDLE> handle_from_path_;
  std::map<HANDLE, std::string> path_from_handle_;
  std::vector<HANDLE> handles_;
  std::vector<HANDLE> pending_close_;
  HANDLE thread_ = nullptr;
  int msg_ = 0;
#endif
};

#endif
