#ifndef STRAWBERRY_MTPCONNECTION_H
#define STRAWBERRY_MTPCONNECTION_H

#include "config.h"
#include "core/song.h"

#ifdef HAVE_MTP
#include <libmtp.h>
#endif

#include <string>
#include <vector>

class MtpConnection {
 public:
  MtpConnection();
  ~MtpConnection();

  MtpConnection(const MtpConnection &) = delete;
  MtpConnection &operator=(const MtpConnection &) = delete;

  bool is_valid() const;
  const std::string &error_text() const { return error_text_; }
#ifdef HAVE_MTP
  LIBMTP_mtpdevice_t *device() const { return device_; }
#endif

  bool OpenBySerial(const std::string &serial);
  bool OpenFirst();
  void Close();
  std::string serial() const;
  std::string friendly_name() const;
  std::vector<Song::FileType> SupportedFiletypes() const;

  static void InitLibMtp();
#ifdef HAVE_MTP
  static std::string ErrorString(LIBMTP_error_number_t error_number);
  static Song::FileType FileTypeFromMtp(LIBMTP_filetype_t type);
  static LIBMTP_filetype_t MtpFileTypeFromSong(Song::FileType type);
#endif

 private:
#ifdef HAVE_MTP
  LIBMTP_mtpdevice_t *device_ = nullptr;
#endif
  std::string error_text_;
  static bool initialized_;
};

#endif
