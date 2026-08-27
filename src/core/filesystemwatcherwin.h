#ifndef STRAWBERRY_FILESYSTEMWATCHERWIN_H
#define STRAWBERRY_FILESYSTEMWATCHERWIN_H

#include "core/filesystemwatcherinterface.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class FileSystemWatcherWinThread;

class FileSystemWatcherWin : public FileSystemWatcherInterface {
 public:
  FileSystemWatcherWin();
  ~FileSystemWatcherWin() override;

  void AddPath(const std::string &path) override;
  void RemovePath(const std::string &path) override;
  void Clear() override;

 private:
  FileSystemWatcherWinThread *ThreadForNewWatch();

  std::vector<std::unique_ptr<FileSystemWatcherWinThread>> threads_;
  std::map<std::string, FileSystemWatcherWinThread *> thread_from_path_;
};

#endif
