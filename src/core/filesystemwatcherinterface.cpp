#include "core/filesystemwatcherinterface.h"

void FileSystemWatcherInterface::AddPaths(const std::vector<std::string> &paths) {
  for (const std::string &path : paths) {
    AddPath(path);
  }
}

void FileSystemWatcherInterface::RemovePaths(const std::vector<std::string> &paths) {
  for (const std::string &path : paths) {
    RemovePath(path);
  }
}
