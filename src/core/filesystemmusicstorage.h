#ifndef STRAWBERRY_FILESYSTEMMUSICSTORAGE_H
#define STRAWBERRY_FILESYSTEMMUSICSTORAGE_H

#include "core/musicstorage.h"

class FilesystemMusicStorage : public MusicStorage {
 public:
  explicit FilesystemMusicStorage(std::string root);

  Song::Source source() const override { return Song::Source::LocalFile; }
  std::string LocalPath() const override { return root_; }
  bool CopyToStorage(const CopyJob &job, std::string &error_text) override;
  bool DeleteFromStorage(const DeleteJob &job) override;

 private:
  std::string root_;
};

#endif
