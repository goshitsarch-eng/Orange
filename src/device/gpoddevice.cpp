#include "device/gpoddevice.h"

#include "config.h"
#include "core/logging.h"
#include "device/gpodloader.h"
#include "utilities/fileutils.h"

#ifdef HAVE_GPOD
#include <gpod/itdb.h>
#endif

bool GPodDevice::CopySongs(const std::string &mount_path, const SongList &songs) {
#ifdef HAVE_GPOD
  if (mount_path.empty()) {
    return false;
  }
  GError *error = nullptr;
  Itdb_iTunesDB *db = itdb_parse(mount_path.c_str(), &error);
  if (!db) {
    if (error) {
      LogWarning("Opening iPod database failed: %s", error->message);
      g_error_free(error);
      error = nullptr;
    }
    db = itdb_new();
    if (db) {
      itdb_set_mountpoint(db, mount_path.c_str());
    }
  }
  if (!db) {
    return false;
  }
  Itdb_Playlist *mpl = itdb_playlist_mpl(db);
  if (!mpl) {
    mpl = itdb_playlist_new("iPod", false);
    itdb_playlist_add(db, mpl, -1);
    itdb_playlist_set_mpl(mpl);
  }
  int copied = 0;
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    if (src.empty() || !FileUtils::Exists(src)) {
      continue;
    }
    Itdb_Track *track = itdb_track_new();
    GPodLoader::SongToTrack(song, track);
    itdb_track_add(db, track, -1);
    itdb_playlist_add_track(mpl, track, -1);
    error = nullptr;
    if (!itdb_cp_track_to_ipod(track, src.c_str(), &error)) {
      if (error) {
        LogWarning("Copying %s to iPod failed: %s", src.c_str(), error->message);
        g_error_free(error);
      }
      itdb_track_remove(track);
      continue;
    }
    ++copied;
  }
  error = nullptr;
  const bool wrote = itdb_write(db, &error);
  if (!wrote && error) {
    LogWarning("Writing iPod database failed: %s", error->message);
    g_error_free(error);
  }
  itdb_free(db);
  LogInfo("Copied %d songs to iPod at %s", copied, mount_path.c_str());
  return copied > 0 && wrote;
#else
  (void)mount_path;
  (void)songs;
  return false;
#endif
}
