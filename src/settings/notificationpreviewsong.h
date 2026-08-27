#ifndef STRAWBERRY_NOTIFICATIONPREVIEWSONG_H
#define STRAWBERRY_NOTIFICATIONPREVIEWSONG_H

#include "core/song.h"

namespace NotificationPreviewSong {

// Qt MainWindow::HandleNotificationPreview fake when the playlist is empty.
inline Song Fake() {
  Song song(Song::Source::LocalFile);
  song.set_valid(true);
  song.set_title("Title");
  song.set_artist("Artist");
  song.set_album("Album");
  song.set_length_nanosec(123000000000LL);
  song.set_genre("Classical");
  song.set_composer("Anonymous");
  song.set_performer("Anonymous");
  song.set_track(1);
  song.set_disc(1);
  song.set_year(2011);
  return song;
}

inline Song FromPlaylist(const SongList &songs) { return songs.empty() ? Fake() : songs.front(); }

}  // namespace NotificationPreviewSong

#endif
