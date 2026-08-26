#include "device/mtpdevice.h"

#include "core/logging.h"
#include "core/standardpaths.h"
#include "device/mtpconnection.h"
#include "device/mtploader.h"
#include "utilities/fileutils.h"

#ifdef HAVE_MTP
#include <libmtp.h>
#endif

#include <cstdlib>

bool MtpDevice::CopySongs(const std::string &serial, const SongList &songs) {
#ifdef HAVE_MTP
  MtpConnection connection;
  if (!connection.OpenBySerial(serial)) {
    LogError("Could not open MTP device for copy: %s", connection.error_text().c_str());
    return false;
  }
  int copied = 0;
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    if (src.empty() || !FileUtils::Exists(src)) {
      continue;
    }
    LIBMTP_track_t *track = LIBMTP_new_track_t();
    MtpLoader::SongToTrack(song, track);
    if (!track->filename || !*track->filename) {
      free(track->filename);
      track->filename = strdup(FileUtils::BaseName(src).c_str());
    }
    const int ret = LIBMTP_Send_Track_From_File(connection.device(), src.c_str(), track, nullptr, nullptr);
    if (ret == 0) {
      ++copied;
    } else {
      LIBMTP_error_t *error = LIBMTP_Get_Errorstack(connection.device());
      if (error && error->error_text) {
        LogError("MTP copy failed: %s", error->error_text);
      }
      LIBMTP_Clear_Errorstack(connection.device());
    }
    LIBMTP_destroy_track_t(track);
  }
  LogInfo("Copied %d songs to MTP device", copied);
  return copied > 0;
#else
  (void)serial;
  (void)songs;
  return false;
#endif
}

bool MtpDevice::DeleteSong(const std::string &serial, const Song &song) {
#ifdef HAVE_MTP
  const uint32_t item_id = MtpLoader::ParseItemId(song.url());
  if (item_id == 0) {
    return false;
  }
  MtpConnection connection;
  if (!connection.OpenBySerial(serial)) {
    return false;
  }
  return LIBMTP_Delete_Object(connection.device(), item_id) == 0;
#else
  (void)serial;
  (void)song;
  return false;
#endif
}

std::string MtpDevice::DownloadTrack(const std::string &url) {
#ifdef HAVE_MTP
  const std::string serial = MtpLoader::ParseHost(url);
  const uint32_t item_id = MtpLoader::ParseItemId(url);
  if (serial.empty() || item_id == 0) {
    return {};
  }
  const std::string dest = FileUtils::Join(StandardPaths::CacheDir(), "mtp-" + std::to_string(item_id));
  if (FileUtils::Exists(dest)) {
    return dest;
  }
  MtpConnection connection;
  if (!connection.OpenBySerial(serial)) {
    return {};
  }
  if (LIBMTP_Get_File_To_File(connection.device(), item_id, dest.c_str(), nullptr, nullptr) != 0) {
    return {};
  }
  return dest;
#else
  (void)url;
  return {};
#endif
}
