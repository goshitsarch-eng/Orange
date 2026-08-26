#ifndef STRAWBERRY_FILEWRITEGUARD_H
#define STRAWBERRY_FILEWRITEGUARD_H

#include <string>

class FileWriteGuard {
 public:
  explicit FileWriteGuard(const std::string &path);
  ~FileWriteGuard();
  FileWriteGuard(const FileWriteGuard &) = delete;
  FileWriteGuard &operator=(const FileWriteGuard &) = delete;
  bool ok() const { return ok_; }

 private:
  std::string path_;
  bool ok_ = false;
};

#endif
