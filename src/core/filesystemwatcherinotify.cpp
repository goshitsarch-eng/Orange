#include "core/filesystemwatcherinotify.h"

#include <glib.h>
#include <glib-unix.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

FileSystemWatcherInotify::FileSystemWatcherInotify() {
  inotify_fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (inotify_fd_ >= 0) {
    watch_id_ = g_unix_fd_add(inotify_fd_, G_IO_IN, OnFd, this);
  }
}

FileSystemWatcherInotify::~FileSystemWatcherInotify() { Clear(); }

void FileSystemWatcherInotify::AddPath(const std::string &path) {
  if (inotify_fd_ < 0 || path.empty() || wd_from_path_.count(path)) {
    return;
  }
  const int wd = inotify_add_watch(inotify_fd_, path.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
  if (wd < 0) {
    return;
  }
  wd_from_path_[path] = wd;
  path_from_wd_[wd] = path;
}

void FileSystemWatcherInotify::RemovePath(const std::string &path) {
  auto it = wd_from_path_.find(path);
  if (it == wd_from_path_.end()) {
    return;
  }
  if (inotify_fd_ >= 0) {
    inotify_rm_watch(inotify_fd_, it->second);
  }
  path_from_wd_.erase(it->second);
  wd_from_path_.erase(it);
}

void FileSystemWatcherInotify::Clear() {
  if (watch_id_) {
    g_source_remove(watch_id_);
    watch_id_ = 0;
  }
  if (inotify_fd_ >= 0) {
    for (const auto &entry : wd_from_path_) {
      inotify_rm_watch(inotify_fd_, entry.second);
    }
    close(inotify_fd_);
    inotify_fd_ = -1;
  }
  wd_from_path_.clear();
  path_from_wd_.clear();
}

gboolean FileSystemWatcherInotify::OnFd(gint, GIOCondition, gpointer data) {
  static_cast<FileSystemWatcherInotify *>(data)->OnReadable();
  return G_SOURCE_CONTINUE;
}

void FileSystemWatcherInotify::OnReadable() {
  alignas(struct inotify_event) char buf[4096];
  while (true) {
    const ssize_t len = read(inotify_fd_, buf, sizeof(buf));
    if (len <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      return;
    }
    ssize_t offset = 0;
    while (offset < len) {
      auto *event = reinterpret_cast<struct inotify_event *>(buf + offset);
      auto it = path_from_wd_.find(event->wd);
      if (it != path_from_wd_.end()) {
        PathChanged.Emit(it->second);
      }
      offset += static_cast<ssize_t>(sizeof(struct inotify_event) + event->len);
    }
  }
}
