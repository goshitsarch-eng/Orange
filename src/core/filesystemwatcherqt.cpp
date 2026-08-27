#include "core/filesystemwatcherqt.h"

#include <algorithm>

void FileSystemWatcherQt::AddPath(const std::string &path) {
  if (!path.empty()) {
    paths_.push_back(path);
  }
}
void FileSystemWatcherQt::RemovePath(const std::string &path) {
  paths_.erase(std::remove(paths_.begin(), paths_.end(), path), paths_.end());
}
void FileSystemWatcherQt::Clear() { paths_.clear(); }
