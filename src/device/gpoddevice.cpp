#include "device/gpoddevice.h"

#include "config.h"
#include "core/logging.h"
#include "device/devicecopy.h"
#include "device/gpodcover.h"
#include "device/gpoddelete.h"
#include "device/devicecopyrefresh.h"
#include "device/gpodloader.h"
#include "utilities/fileutils.h"

#ifdef HAVE_GPOD
#include <gpod/itdb.h>
#endif

GPodCopySession::~GPodCopySession() {
#ifdef HAVE_GPOD
  if (db_) {
    itdb_free(static_cast<Itdb_iTunesDB *>(db_));
    db_ = nullptr;
    mpl_ = nullptr;
  }
#endif
}

bool GPodCopySession::Open(const std::string &mount_path) {
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
  db_ = db;
  mpl_ = mpl;
  mount_path_ = mount_path;
  copied_ = 0;
  return true;
#else
  (void)mount_path;
  return false;
#endif
}

bool GPodCopySession::CopyOne(const Song &song, const std::string &playlist, const std::string &cover_source, Song *on_device) {
#ifdef HAVE_GPOD
  auto *db = static_cast<Itdb_iTunesDB *>(db_);
  auto *mpl = static_cast<Itdb_Playlist *>(mpl_);
  if (!db || !mpl) {
    return false;
  }
  const std::string src = FileUtils::PathFromUri(song.url());
  if (src.empty() || !FileUtils::Exists(src)) {
    return false;
  }
  Itdb_Track *track = itdb_track_new();
  GPodLoader::SongToTrack(song, track);
  itdb_track_add(db, track, -1);
  itdb_playlist_add_track(mpl, track, -1);
  if (GPodCover::ShouldSetThumbnails(true, cover_source) && FileUtils::IsFile(cover_source)) {
    if (itdb_track_set_thumbnails(track, cover_source.c_str())) {
      track->has_artwork = 1;
    } else {
      LogWarning("Failed to set album cover image");
    }
  }
  GError *error = nullptr;
  if (!itdb_cp_track_to_ipod(track, src.c_str(), &error)) {
    if (error) {
      LogWarning("Copying %s to iPod failed: %s", src.c_str(), error->message);
      g_error_free(error);
    }
    itdb_track_remove(track);
    return false;
  }
  if (DeviceCopyPlaylist::ShouldWriteNamedPlaylist(playlist)) {
    std::string name = playlist;
    Itdb_Playlist *named = itdb_playlist_by_name(db, name.data());
    if (!named) {
      named = itdb_playlist_new(name.c_str(), FALSE);
      itdb_playlist_add(db, named, -1);
    }
    itdb_playlist_add_track(named, track, -1);
  }
  if (on_device) {
    *on_device = GPodLoader::SongFromTrack(track, mount_path_);
    DeviceCopyRefresh::ApplyGPodCollectionFields(on_device);
  }
  ++copied_;
  return true;
#else
  (void)song;
  (void)playlist;
  (void)cover_source;
  (void)on_device;
  return false;
#endif
}

bool GPodCopySession::Finish() {
#ifdef HAVE_GPOD
  auto *db = static_cast<Itdb_iTunesDB *>(db_);
  if (!db) {
    return false;
  }
  GError *error = nullptr;
  const bool wrote = itdb_write(db, &error);
  if (!wrote && error) {
    LogWarning("Writing iPod database failed: %s", error->message);
    g_error_free(error);
  }
  itdb_free(db);
  db_ = nullptr;
  mpl_ = nullptr;
  return wrote;
#else
  return false;
#endif
}

bool GPodDevice::CopyOne(const std::string &mount_path, const Song &song) {
  GPodCopySession session;
  if (!session.Open(mount_path) || !session.CopyOne(song)) {
    return false;
  }
  return session.Finish();
}

bool GPodDevice::DeleteSong(const std::string &mount_path, const Song &song) {
#ifdef HAVE_GPOD
  if (mount_path.empty()) {
    return false;
  }
  GError *error = nullptr;
  Itdb_iTunesDB *db = itdb_parse(mount_path.c_str(), &error);
  if (!db) {
    if (error) {
      LogWarning("Loading iPod database failed: %s", error->message);
      g_error_free(error);
    }
    return false;
  }
  const std::string ipod_path = GPodDelete::IpodPathFromUrl(song.url(), mount_path);
  Itdb_Track *track = nullptr;
  for (GList *tracks = db->tracks; tracks; tracks = tracks->next) {
    auto *candidate = static_cast<Itdb_Track *>(tracks->data);
    if (candidate && GPodDelete::TrackMatches(candidate->ipod_path, ipod_path)) {
      track = candidate;
      break;
    }
  }
  if (!track) {
    LogWarning("Couldn't find song %s in iTunesDB", song.url().c_str());
    itdb_free(db);
    return false;
  }
  for (GList *playlists = db->playlists; playlists; playlists = playlists->next) {
    auto *playlist = static_cast<Itdb_Playlist *>(playlists->data);
    if (playlist && itdb_playlist_contains_track(playlist, track)) {
      itdb_playlist_remove_track(playlist, track);
    }
  }
  itdb_track_remove(track);
  const std::string local = GPodDelete::LocalPath(song.url());
  if (local.empty() || !FileUtils::Remove(local)) {
    itdb_free(db);
    return false;
  }
  error = nullptr;
  const bool wrote = itdb_write(db, &error);
  if (!wrote && error) {
    LogWarning("Writing iPod database failed: %s", error->message);
    g_error_free(error);
  }
  itdb_free(db);
  return wrote;
#else
  (void)mount_path;
  (void)song;
  return false;
#endif
}

bool GPodDevice::CopySongs(const std::string &mount_path, const SongList &songs) {
#ifdef HAVE_GPOD
  GPodCopySession session;
  if (!session.Open(mount_path)) {
    return false;
  }
  for (const Song &song : songs) {
    session.CopyOne(song);
  }
  const bool wrote = session.Finish();
  LogInfo("Copied %d songs to iPod at %s", session.copied(), mount_path.c_str());
  return session.copied() > 0 && wrote;
#else
  (void)mount_path;
  (void)songs;
  return false;
#endif
}
