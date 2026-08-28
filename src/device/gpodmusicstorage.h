#ifndef STRAWBERRY_GPODMUSICSTORAGE_H
#define STRAWBERRY_GPODMUSICSTORAGE_H

#include "core/musicstorage.h"
#include "core/song.h"
#include "device/gpoddevice.h"

#include <string>
#include <vector>

class GPodMusicStorage : public MusicStorage {
 public:
  explicit GPodMusicStorage(std::string mount_path);

  Song::Source source() const override { return Song::Source::Device; }
  std::string LocalPath() const override { return mount_path_; }
  bool StartCopy(std::vector<Song::FileType> *supported) override;
  bool CopyToStorage(const CopyJob &job, std::string &error_text) override;
  bool FinishCopy(bool success, std::string &error_text) override;
  bool DeleteFromStorage(const DeleteJob &job) override;
  SongList CopiedSongs() const override { return copied_songs_; }

 private:
  std::string mount_path_;
  GPodCopySession session_;
  SongList copied_songs_;
  bool opened_ = false;
};

#endif
