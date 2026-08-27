#ifndef STRAWBERRY_DEVICECOPYSUPPORTED_H
#define STRAWBERRY_DEVICECOPYSUPPORTED_H

#include "config.h"
#include "core/song.h"
#include "device/connecteddevice.h"
#include "device/devicecopyjob.h"
#include "device/mtpconnection.h"
#include "organize/organizetranscode.h"

#ifdef HAVE_MTP
#include <libmtp.h>
#endif

#include <string>
#include <vector>

namespace DeviceCopySupported {

// Qt MtpDevice::GetSupportedFiletypes
#ifdef HAVE_MTP
inline void AppendFromMtp(LIBMTP_filetype_t type, std::vector<Song::FileType> *out) {
  if (!out) {
    return;
  }
  switch (type) {
    case LIBMTP_FILETYPE_WAV:
      out->push_back(Song::FileType::WAV);
      break;
    case LIBMTP_FILETYPE_MP2:
    case LIBMTP_FILETYPE_MP3:
      out->push_back(Song::FileType::MPEG);
      break;
    case LIBMTP_FILETYPE_WMA:
      out->push_back(Song::FileType::ASF);
      break;
    case LIBMTP_FILETYPE_MP4:
    case LIBMTP_FILETYPE_M4A:
    case LIBMTP_FILETYPE_AAC:
      out->push_back(Song::FileType::MP4);
      break;
    case LIBMTP_FILETYPE_FLAC:
      out->push_back(Song::FileType::FLAC);
      out->push_back(Song::FileType::OggFlac);
      break;
    case LIBMTP_FILETYPE_OGG:
      out->push_back(Song::FileType::OggVorbis);
      out->push_back(Song::FileType::OggSpeex);
      out->push_back(Song::FileType::OggFlac);
      break;
    default:
      break;
  }
}
#endif

// Prefer the device's queried list (Qt StartCopy / GetSupportedFiletypes). Empty query falls back to the backend default.
inline std::vector<Song::FileType> ForCopy(const std::string &backend, const std::vector<Song::FileType> &queried) {
  if (!queried.empty()) {
    return queried;
  }
  return OrganizeTranscode::SupportedForBackend(backend);
}

inline std::vector<Song::FileType> Query(const ConnectedDevice &device) {
#ifdef HAVE_MTP
  if (device.backend == "mtp") {
    MtpConnection connection;
    if (connection.OpenBySerial(DeviceCopyJob::MtpSerial(device.unique_id))) {
      return connection.SupportedFiletypes();
    }
  }
#else
  (void)device;
#endif
  return {};
}

inline std::vector<Song::FileType> ForDevice(const ConnectedDevice &device) { return ForCopy(device.backend, Query(device)); }

}  // namespace DeviceCopySupported

#endif
