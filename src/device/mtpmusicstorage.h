#ifndef STRAWBERRY_MTPMUSICSTORAGE_H
#define STRAWBERRY_MTPMUSICSTORAGE_H

#include "core/musicstorage.h"
#include "core/song.h"
#include "device/mtpdevice.h"

#include <string>
#include <vector>

class MtpMusicStorage : public MusicStorage {
 public:
  explicit MtpMusicStorage(std::string serial);

  Song::Source source() const override { return Song::Source::Device; }
  std::string LocalPath() const override { return {}; }
  bool StartCopy(std::vector<Song::FileType> *supported) override;
  bool CopyToStorage(const CopyJob &job, std::string &error_text) override;
  bool FinishCopy(bool success, std::string &error_text) override;
  bool DeleteFromStorage(const DeleteJob &job) override;
  SongList CopiedSongs() const override { return copied_songs_; }

 private:
  std::string serial_;
  MtpCopySession session_;
  SongList copied_songs_;
};

#endif
