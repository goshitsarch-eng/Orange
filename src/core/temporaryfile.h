#ifndef STRAWBERRY_TEMPORARYFILE_H
#define STRAWBERRY_TEMPORARYFILE_H

#include <string>

class TemporaryFile {
 public:
  explicit TemporaryFile(const std::string &filename_pattern = "strawberry-XXXXXX");
  ~TemporaryFile();

  const std::string &filename() const { return filename_; }

 private:
  std::string filename_;
};

#endif
