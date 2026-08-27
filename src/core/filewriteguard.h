#ifndef STRAWBERRY_FILEWRITEGUARD_H
#define STRAWBERRY_FILEWRITEGUARD_H

#include <string>

class FileWriteGuard {
 public:
  explicit FileWriteGuard(const std::string &path);
  ~FileWriteGuard();
  FileWriteGuard(const FileWriteGuard &) = delete;
  FileWriteGuard &operator=(const FileWriteGuard &) = delete;

  bool Init();
  bool Commit();
  bool ok() const { return ok_; }
  bool active() const { return active_; }
  const std::string &working_filename() const { return working_; }

 private:
  std::string path_;
  std::string working_;
  std::string temp_path_;
  bool ok_ = false;
  bool active_ = false;
  bool committed_ = false;
};

#endif
