#ifndef STRAWBERRY_MPRIS2_H
#define STRAWBERRY_MPRIS2_H

#include <gio/gio.h>

#include "core/song.h"

#include <cstdint>
#include <string>
#include <vector>

class Application;

class Mpris2 {
 public:
  explicit Mpris2(Application *app);
  ~Mpris2();
  void EmitSeeked(int64_t position_us);
  void EmitPosition();
  void EmitPlaybackStatus();
  void EmitMetadata();
  void EmitVolume();
  void EmitTrackListReplaced();
  Application *app() const { return app_; }

 private:
  static void OnBusAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
  void EmitPropertiesChanged(const char *interface_name, const char *property, GVariant *value);
  void WatchCurrentPlaylist();
  void OnPlaylistContentsChanged();
  void SnapshotTrackList();
  std::vector<std::string> CurrentTrackIds() const;
  SongList CurrentSongs() const;

  Application *app_;
  guint owner_id_ = 0;
  GDBusConnection *connection_ = nullptr;
  int playlist_watch_gen_ = 0;
  std::vector<std::string> last_track_ids_;
  SongList last_songs_;
};

#endif
