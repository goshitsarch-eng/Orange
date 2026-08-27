#ifndef STRAWBERRY_PLAYLISTITEMBIND_H
#define STRAWBERRY_PLAYLISTITEMBIND_H

#include "core/database.h"
#include "core/song.h"
#include "playlist/playlistitemuuid.h"

#include <string>

namespace PlaylistItemBind {

inline const char *InsertSql() {
  return "INSERT INTO playlist_items (playlist, type, uuid, collection_id, title, titlesort, album, albumsort, artist, "
         "artistsort, albumartist, albumartistsort, track, disc, year, originalyear, genre, compilation, composer, "
         "composersort, performer, performersort, grouping, comment, lyrics, beginning, length, bitrate, samplerate, "
         "bitdepth, source, directory_id, url, filetype, filesize, mtime, ctime, unavailable, fingerprint, playcount, "
         "skipcount, lastplayed, rating, cue_path, bpm, mood, initial_key, art_embedded, art_automatic, art_manual) "
         "VALUES (?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
         "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
}

inline const char *UpdateSql() {
  return "UPDATE playlist_items SET collection_id=?, title=?, album=?, artist=?, albumartist=?, track=?, disc=?, year=?, "
         "originalyear=?, genre=?, composer=?, comment=?, lyrics=?, length=?, rating=?, cue_path=?, bpm=?, mood=?, "
         "initial_key=? WHERE playlist=? AND uuid=?";
}

inline const char *LoadSql() {
  return "SELECT collection_id, uuid, url, title, album, artist, albumartist, track, disc, year, originalyear, genre, "
         "composer, comment, lyrics, length, rating, cue_path, bpm, mood, initial_key, source FROM playlist_items "
         "WHERE playlist = ?";
}

inline void BindInsert(SqlQuery *query, int playlist_id, const std::string &uuid, const Song &song) {
  if (!query) {
    return;
  }
  query->Bind(1, playlist_id);
  query->Bind(2, PlaylistItemUuid::Valid(uuid) ? uuid : PlaylistItemUuid::New());
  query->Bind(3, song.id());
  query->Bind(4, song.title());
  query->Bind(5, song.titlesort());
  query->Bind(6, song.album());
  query->Bind(7, song.albumsort());
  query->Bind(8, song.artist());
  query->Bind(9, song.artistsort());
  query->Bind(10, song.albumartist());
  query->Bind(11, song.albumartistsort());
  query->Bind(12, song.track());
  query->Bind(13, song.disc());
  query->Bind(14, song.year());
  query->Bind(15, song.originalyear());
  query->Bind(16, song.genre());
  query->Bind(17, song.compilation() ? 1 : 0);
  query->Bind(18, song.composer());
  query->Bind(19, song.composersort());
  query->Bind(20, song.performer());
  query->Bind(21, song.performersort());
  query->Bind(22, song.grouping());
  query->Bind(23, song.comment());
  query->Bind(24, song.lyrics());
  query->Bind(25, song.beginning_nanosec());
  query->Bind(26, song.length_nanosec());
  query->Bind(27, song.bitrate());
  query->Bind(28, song.samplerate());
  query->Bind(29, song.bitdepth());
  query->Bind(30, static_cast<int>(song.source()));
  query->Bind(31, song.directory_id());
  query->Bind(32, song.url());
  query->Bind(33, static_cast<int>(song.filetype()));
  query->Bind(34, song.filesize());
  query->Bind(35, song.mtime());
  query->Bind(36, song.ctime());
  query->Bind(37, song.unavailable() ? 1 : 0);
  query->Bind(38, song.fingerprint());
  query->Bind(39, static_cast<int>(song.playcount()));
  query->Bind(40, static_cast<int>(song.skipcount()));
  query->Bind(41, song.lastplayed());
  query->Bind(42, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
  query->Bind(43, song.cue_path());
  query->Bind(44, static_cast<double>(song.bpm()));
  query->Bind(45, song.mood());
  query->Bind(46, song.initial_key());
  query->Bind(47, song.art_embedded() ? 1 : 0);
  query->Bind(48, song.art_automatic());
  query->Bind(49, song.art_manual());
}

inline void BindUpdate(SqlQuery *query, int playlist_id, const std::string &uuid, const Song &song) {
  if (!query) {
    return;
  }
  query->Bind(1, song.id());
  query->Bind(2, song.title());
  query->Bind(3, song.album());
  query->Bind(4, song.artist());
  query->Bind(5, song.albumartist());
  query->Bind(6, song.track());
  query->Bind(7, song.disc());
  query->Bind(8, song.year());
  query->Bind(9, song.originalyear());
  query->Bind(10, song.genre());
  query->Bind(11, song.composer());
  query->Bind(12, song.comment());
  query->Bind(13, song.lyrics());
  query->Bind(14, song.length_nanosec());
  query->Bind(15, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
  query->Bind(16, song.cue_path());
  query->Bind(17, static_cast<double>(song.bpm()));
  query->Bind(18, song.mood());
  query->Bind(19, song.initial_key());
  query->Bind(20, playlist_id);
  query->Bind(21, uuid);
}

}  // namespace PlaylistItemBind

#endif  // STRAWBERRY_PLAYLISTITEMBIND_H
