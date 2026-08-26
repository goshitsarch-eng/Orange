#ifndef STRAWBERRY_FILESYSTEMWATCHERINOTIFY_H
#define STRAWBERRY_FILESYSTEMWATCHERINOTIFY_H

#include "core/filesystemwatcherinterface.h"

#include <glib.h>

#include <map>
#include <string>
#include <vector>

class FileSystemWatcherInotify : public FileSystemWatcherInterface {
 public:
  FileSystemWatcherInotify();
  ~FileSystemWatcherInotify() override;

  void AddPath(const std::string &path) override;
  void RemovePath(const std::string &path) override;
  void Clear() override;

 private:
  void OnReadable();
  static gboolean OnFd(gint fd, GIOCondition condition, gpointer data);

  int inotify_fd_ = -1;
  unsigned watch_id_ = 0;
  std::map<std::string, int> wd_from_path_;
  std::map<int, std::string> path_from_wd_;
};

#endif
