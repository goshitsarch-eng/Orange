#ifndef STRAWBERRY_FILESYSTEMWATCHERINTERFACE_H
#define STRAWBERRY_FILESYSTEMWATCHERINTERFACE_H

#include "core/signal.h"

#include <string>
#include <vector>

class FileSystemWatcherInterface {
 public:
  virtual ~FileSystemWatcherInterface() = default;
  virtual void AddPath(const std::string &path) = 0;
  virtual void AddPaths(const std::vector<std::string> &paths);
  virtual void RemovePath(const std::string &path) = 0;
  virtual void RemovePaths(const std::vector<std::string> &paths);
  virtual void Clear() = 0;

  Signal<std::string> PathChanged;
};

#endif
