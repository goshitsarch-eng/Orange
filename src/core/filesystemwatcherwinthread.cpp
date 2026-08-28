#include "core/filesystemwatcherwinthread.h"

#include "core/filesystemwatcherwinpolicy.h"

#include <glib.h>
#include <string>

#ifdef _WIN32
#include <process.h>
#endif

#ifdef _WIN32
namespace {

struct PathChangedIdle {
  FileSystemWatcherWinThread *self = nullptr;
  std::string path;
  std::shared_ptr<bool> alive;
  bool dropped = false;
};

gboolean EmitPathChangedIdle(gpointer data) {
  auto *idle = static_cast<PathChangedIdle *>(data);
  if (idle->alive && *idle->alive) {
    if (idle->dropped) {
      idle->self->WatchDropped.Emit(idle->path);
    }
    idle->self->PathChanged.Emit(idle->path);
  }
  delete idle;
  return G_SOURCE_REMOVE;
}

std::wstring Utf8ToWide(const std::string &path) {
  if (path.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
  if (n <= 0) {
    return {};
  }
  std::wstring wide(static_cast<size_t>(n - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), n);
  return wide;
}

}  // namespace
#endif

FileSystemWatcherWinThread::FileSystemWatcherWinThread() {
#ifdef _WIN32
  handles_.reserve(FileSystemWatcherWinPolicy::kMaxWaitObjects);
  wakeup_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  handles_.push_back(wakeup_);
  thread_ = CreateThread(nullptr, 0, &FileSystemWatcherWinThread::ThreadProc, this, 0, nullptr);
#endif
}

FileSystemWatcherWinThread::~FileSystemWatcherWinThread() { Stop(); }

#ifdef _WIN32
DWORD WINAPI FileSystemWatcherWinThread::ThreadProc(LPVOID self) {
  static_cast<FileSystemWatcherWinThread *>(self)->Run();
  return 0;
}
#endif

bool FileSystemWatcherWinThread::AddPath(const std::string &path) {
#ifdef _WIN32
  std::lock_guard<std::mutex> lock(mutex_);
  if (!FileSystemWatcherWinPolicy::ThreadHasRoom(static_cast<int>(handle_from_path_.size())) || handle_from_path_.count(path)) {
    return false;
  }
  const std::wstring wide = Utf8ToWide(path);
  HANDLE handle = FindFirstChangeNotificationW(wide.c_str(), FileSystemWatcherWinPolicy::kWatchSubtree ? TRUE : FALSE,
                                               FileSystemWatcherWinPolicy::kNotifyFlags);
  if (handle == INVALID_HANDLE_VALUE || handle == nullptr) {
    return false;
  }
  handle_from_path_[path] = handle;
  path_from_handle_[handle] = path;
  handles_.push_back(handle);
  msg_ = '@';
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
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = handle_from_path_.find(path);
  if (it == handle_from_path_.end()) {
    return false;
  }
  const HANDLE handle = it->second;
  handle_from_path_.erase(it);
  path_from_handle_.erase(handle);
  for (auto hit = handles_.begin(); hit != handles_.end(); ++hit) {
    if (*hit == handle) {
      handles_.erase(hit);
      break;
    }
  }
  pending_close_.push_back(handle);
  msg_ = '@';
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
  std::lock_guard<std::mutex> lock(mutex_);
  return handle_from_path_.empty();
#else
  return true;
#endif
}

int FileSystemWatcherWinThread::WatchCount() const {
#ifdef _WIN32
  std::lock_guard<std::mutex> lock(mutex_);
  return static_cast<int>(handle_from_path_.size());
#else
  return 0;
#endif
}

void FileSystemWatcherWinThread::Stop() {
#ifdef _WIN32
  *alive_ = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    msg_ = 'q';
    if (wakeup_) {
      SetEvent(wakeup_);
    }
  }
  if (thread_) {
    WaitForSingleObject(thread_, INFINITE);
    CloseHandle(thread_);
    thread_ = nullptr;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  ClosePending();
  for (size_t i = 1; i < handles_.size(); ++i) {
    FindCloseChangeNotification(handles_[i]);
  }
  handles_.clear();
  handle_from_path_.clear();
  path_from_handle_.clear();
  if (wakeup_) {
    CloseHandle(wakeup_);
    wakeup_ = nullptr;
  }
#endif
}

#ifdef _WIN32
void FileSystemWatcherWinThread::ClosePending() {
  for (HANDLE handle : pending_close_) {
    FindCloseChangeNotification(handle);
  }
  pending_close_.clear();
}
#endif

void FileSystemWatcherWinThread::SchedulePathChanged(const std::string &path, bool dropped) {
#ifdef _WIN32
  auto *idle = new PathChangedIdle;
  idle->self = this;
  idle->path = path;
  idle->alive = alive_;
  idle->dropped = dropped;
  g_idle_add(EmitPathChangedIdle, idle);
#else
  (void)path;
  (void)dropped;
#endif
}

void FileSystemWatcherWinThread::Run() {
#ifdef _WIN32
  std::unique_lock<std::mutex> lock(mutex_);
  while (true) {
    const std::vector<HANDLE> handles = handles_;
    lock.unlock();

    const DWORD result = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);

    lock.lock();
    ClosePending();

    if (result == WAIT_FAILED || result < WAIT_OBJECT_0 || result >= WAIT_OBJECT_0 + static_cast<DWORD>(handles.size())) {
      break;
    }

    const DWORD index = result - WAIT_OBJECT_0;
    if (index == 0) {
      const int msg = msg_;
      msg_ = 0;
      if (msg == 'q') {
        return;
      }
      continue;
    }

    const HANDLE handle = handles[index];
    auto path_it = path_from_handle_.find(handle);
    if (path_it == path_from_handle_.end()) {
      continue;
    }
    const std::string path = path_it->second;
    bool dropped = false;
    if (!FindNextChangeNotification(handle)) {
      FindCloseChangeNotification(handle);
      path_from_handle_.erase(handle);
      handle_from_path_.erase(path);
      for (auto hit = handles_.begin(); hit != handles_.end(); ++hit) {
        if (*hit == handle) {
          handles_.erase(hit);
          break;
        }
      }
      dropped = true;
    }

    lock.unlock();
    SchedulePathChanged(path, dropped);
    lock.lock();
  }
#endif
}
