#ifndef STRAWBERRY_FILESYSTEMWATCHERQT_H
#define STRAWBERRY_FILESYSTEMWATCHERQT_H

#include "core/filesystemwatcherinterface.h"

#include <string>
#include <vector>

class FileSystemWatcherQt : public FileSystemWatcherInterface {
 public:
  void AddPath(const std::string &path) override;
  void RemovePath(const std::string &path) override;
  void Clear() override;

 private:
  std::vector<std::string> paths_;
};

#endif
