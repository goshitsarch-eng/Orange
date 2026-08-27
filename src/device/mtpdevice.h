#ifndef STRAWBERRY_MTPDEVICE_H
#define STRAWBERRY_MTPDEVICE_H

#include "config.h"
#include "core/song.h"
#include "device/mtpconnection.h"

#include <memory>
#include <string>

class MtpCopySession {
 public:
  bool Open(const std::string &serial);
  bool CopyOne(const Song &song);
  void Close();
  bool is_open() const;

 private:
  std::unique_ptr<MtpConnection> connection_;
};

namespace MtpDevice {

bool CopySongs(const std::string &serial, const SongList &songs);
bool CopyOne(const std::string &serial, const Song &song);
bool DeleteSong(const std::string &serial, const Song &song);
std::string DownloadTrack(const std::string &url);

}  // namespace MtpDevice

#endif
