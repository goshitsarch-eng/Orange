#ifndef STRAWBERRY_GPODDEVICE_H
#define STRAWBERRY_GPODDEVICE_H

#include "core/song.h"

#include <string>

class GPodCopySession {
 public:
  GPodCopySession() = default;
  ~GPodCopySession();
  GPodCopySession(const GPodCopySession &) = delete;
  GPodCopySession &operator=(const GPodCopySession &) = delete;

  bool Open(const std::string &mount_path);
  bool CopyOne(const Song &song, const std::string &playlist = {}, const std::string &cover_source = {}, Song *on_device = nullptr);
  bool Finish();
  int copied() const { return copied_; }

 private:
  void *db_ = nullptr;
  void *mpl_ = nullptr;
  std::string mount_path_;
  int copied_ = 0;
};

namespace GPodDevice {

bool CopySongs(const std::string &mount_path, const SongList &songs);
bool CopyOne(const std::string &mount_path, const Song &song);
bool DeleteSong(const std::string &mount_path, const Song &song);

}  // namespace GPodDevice

#endif
