#include "core/filesystemwatcherwinthread.h"

#include "core/filesystemwatcherwinpolicy.h"

#ifdef _WIN32
#include <process.h>
#endif

FileSystemWatcherWinThread::FileSystemWatcherWinThread() {
#ifdef _WIN32
  wakeup_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
#endif
}

FileSystemWatcherWinThread::~FileSystemWatcherWinThread() { Stop(); }

bool FileSystemWatcherWinThread::AddPath(const std::string &path) {
#ifdef _WIN32
  if (!FileSystemWatcherWinPolicy::ThreadHasRoom(WatchCount()) || handle_from_path_.count(path)) {
    return false;
  }
  std::wstring wide(path.begin(), path.end());
  HANDLE handle = FindFirstChangeNotificationW(wide.c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                                                       FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  handle_from_path_[path] = handle;
  if (wakeup_) {
    SetEvent(wakeup_);
  }
  return true;
#else
  (void)path;
  return false;
#endif
}

bool FileSystemWatcherWinThread::RemovePath(const std::string &path) {
#ifdef _WIN32
  auto it = handle_from_path_.find(path);
  if (it == handle_from_path_.end()) {
    return false;
  }
  FindCloseChangeNotification(it->second);
  handle_from_path_.erase(it);
  if (wakeup_) {
    SetEvent(wakeup_);
  }
  return true;
#else
  (void)path;
  return false;
#endif
}

bool FileSystemWatcherWinThread::IsEmpty() const {
#ifdef _WIN32
  return handle_from_path_.empty();
#else
  return true;
#endif
}

int FileSystemWatcherWinThread::WatchCount() const {
#ifdef _WIN32
  return static_cast<int>(handle_from_path_.size());
#else
  return 0;
#endif
}

void FileSystemWatcherWinThread::Stop() {
#ifdef _WIN32
  stop_ = true;
  if (wakeup_) {
    SetEvent(wakeup_);
  }
  for (auto &entry : handle_from_path_) {
    FindCloseChangeNotification(entry.second);
  }
  handle_from_path_.clear();
  if (wakeup_) {
    CloseHandle(wakeup_);
    wakeup_ = nullptr;
  }
#endif
}

void FileSystemWatcherWinThread::Run() {
#ifdef _WIN32
  while (!stop_) {
    std::vector<HANDLE> handles;
    std::vector<std::string> paths;
    handles.push_back(wakeup_);
    paths.emplace_back();
    for (const auto &entry : handle_from_path_) {
      handles.push_back(entry.second);
      paths.push_back(entry.first);
    }
    const DWORD result = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
    if (result == WAIT_OBJECT_0 || result == WAIT_FAILED) {
      continue;
    }
    const DWORD index = result - WAIT_OBJECT_0;
    if (index > 0 && index < handles.size()) {
      FindNextChangeNotification(handles[index]);
      PathChanged.Emit(paths[index]);
    }
  }
#endif
}
