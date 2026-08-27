#ifndef STRAWBERRY_FILESYSTEMWATCHERWINTHREAD_H
#define STRAWBERRY_FILESYSTEMWATCHERWINTHREAD_H

#include "core/signal.h"

#include <map>
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

 private:
  void Run();

#ifdef _WIN32
  HANDLE wakeup_ = nullptr;
  std::map<std::string, HANDLE> handle_from_path_;
  void *thread_ = nullptr;
  bool stop_ = false;
#endif
};

#endif
