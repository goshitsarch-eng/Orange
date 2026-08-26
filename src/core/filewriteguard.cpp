#include "core/filewriteguard.h"

#include "utilities/fileutils.h"

#include <glib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <vector>

FileWriteGuard::FileWriteGuard(const std::string &path)
    : path_(path), working_(path), active_(FileUtils::FilenameOnGVFS(path)) {
  ok_ = !path.empty() && (active_ || access(path.c_str(), W_OK) == 0);
}

FileWriteGuard::~FileWriteGuard() {
  if (active_ && !committed_ && !temp_path_.empty()) {
    FileUtils::Remove(temp_path_);
  }
}

bool FileWriteGuard::Init() {
  if (path_.empty()) {
    ok_ = false;
    return false;
  }
  if (!active_) {
    ok_ = access(path_.c_str(), W_OK) == 0;
    working_ = path_;
    return ok_;
  }

  const std::string ext = FileUtils::Extension(path_);
  const std::string suffix = ext.empty() ? std::string() : ("." + ext);
  std::string tmpl = std::string(g_get_tmp_dir()) + "/strawberry_tag_XXXXXX" + suffix;
  std::vector<char> buffer(tmpl.begin(), tmpl.end());
  buffer.push_back('\0');
  const int fd = mkstemps(buffer.data(), static_cast<int>(suffix.size()));
  if (fd < 0) {
    ok_ = false;
    return false;
  }
  close(fd);
  temp_path_ = buffer.data();
  working_ = temp_path_;
  ok_ = FileUtils::CopyFile(path_, working_);
  return ok_;
}

bool FileWriteGuard::Commit() {
  if (!active_) {
    committed_ = true;
    return true;
  }
  if (working_.empty() || path_.empty()) {
    return false;
  }

  const std::string dir = FileUtils::DirName(path_);
  std::string tmpl = (dir.empty() ? std::string(".") : dir) + "/.strawberry_tag_XXXXXX";
  std::vector<char> buffer(tmpl.begin(), tmpl.end());
  buffer.push_back('\0');
  const int fd = mkstemp(buffer.data());
  if (fd < 0) {
    return false;
  }
  close(fd);
  const std::string sibling = buffer.data();
  if (!FileUtils::CopyFile(working_, sibling)) {
    FileUtils::Remove(sibling);
    return false;
  }

  struct stat st {};
  if (stat(path_.c_str(), &st) == 0) {
    chmod(sibling.c_str(), st.st_mode);
  }
  if (std::rename(sibling.c_str(), path_.c_str()) != 0) {
    FileUtils::Remove(sibling);
    return false;
  }
  committed_ = true;
  return true;
}
