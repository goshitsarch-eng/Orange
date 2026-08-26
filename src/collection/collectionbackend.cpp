#include "collection/collectionbackend.h"

#include "collection/collectioncompilation.h"
#include "collection/collectionquery.h"
#include "filterparser/filterparser.h"
#include "utilities/strutils.h"

#include <algorithm>

CollectionBackend::CollectionBackend(Database *database) : database_(database) {}

std::vector<CollectionDirectory> CollectionBackend::Directories() const {
  std::vector<CollectionDirectory> result;
  if (!database_ || !database_->handle()) {
    return result;
  }
  SqlQuery query(database_, "SELECT rowid, path, subdirs FROM directories ORDER BY path");
  while (query.Step()) {
    CollectionDirectory directory;
    directory.id = query.ColumnInt(0);
    directory.path = query.ColumnText(1);
    directory.subdirs = query.ColumnInt(2) != 0;
    result.push_back(directory);
  }
  return result;
}

int CollectionBackend::AddDirectory(const std::string &path, bool subdirs) {
  SqlQuery query(database_, "INSERT INTO directories (path, subdirs) VALUES (?, ?)");
  query.Bind(1, path);
  query.Bind(2, subdirs ? 1 : 0);
  if (!query.Exec()) {
    return -1;
  }
  DirectoryAdded.Emit();
  return static_cast<int>(database_->LastInsertRowId());
}

void CollectionBackend::RemoveDirectory(int id) {
  DeleteSongsInDirectory(id);
  SqlQuery query(database_, "DELETE FROM directories WHERE rowid = ?");
  query.Bind(1, id);
  query.Exec();
  DirectoryDeleted.Emit();
}

Song CollectionBackend::SongFromQuery(const SqlQuery &query) const {
  Song song(Song::Source::Collection);
  song.set_id(query.ColumnInt(0));
  song.set_title(query.ColumnText(1));
  song.set_album(query.ColumnText(3));
  song.set_artist(query.ColumnText(5));
  song.set_albumartist(query.ColumnText(7));
  song.set_track(query.ColumnInt(9));
  song.set_disc(query.ColumnInt(10));
  song.set_year(query.ColumnInt(11));
  song.set_originalyear(query.ColumnInt(12));
  song.set_genre(query.ColumnText(13));
  song.set_compilation(query.ColumnInt(46) != 0);
  song.set_composer(query.ColumnText(15));
  song.set_performer(query.ColumnText(17));
  song.set_grouping(query.ColumnText(19));
  song.set_comment(query.ColumnText(20));
  song.set_lyrics(query.ColumnText(21));
  song.set_beginning_nanosec(query.ColumnInt64(25));
  song.set_length_nanosec(query.ColumnInt64(26));
  song.set_bitrate(query.ColumnInt(27));
  song.set_samplerate(query.ColumnInt(28));
  song.set_bitdepth(query.ColumnInt(29));
  song.set_source(static_cast<Song::Source>(query.ColumnInt(30)));
  song.set_directory_id(query.ColumnInt(31));
  song.set_url(query.ColumnText(32));
  song.set_filetype(static_cast<Song::FileType>(query.ColumnInt(33)));
  song.set_filesize(query.ColumnInt64(34));
  song.set_mtime(query.ColumnInt64(35));
  song.set_ctime(query.ColumnInt64(36));
  song.set_unavailable(query.ColumnInt(37) != 0);
  song.set_playcount(static_cast<unsigned>(query.ColumnInt(39)));
  song.set_skipcount(static_cast<unsigned>(query.ColumnInt(40)));
  song.set_lastplayed(query.ColumnInt64(41));
  song.set_art_embedded(query.ColumnInt(47) != 0);
  song.set_rating(static_cast<float>(query.ColumnInt(54)) / 100.0f);
  song.set_valid(true);
  return song;
}

SongList CollectionBackend::Songs(const std::string &filter) const {
  SongList songs;
  std::string sql = "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs";
  if (!filter.empty()) {
    const std::string where = FilterParser(filter).ToSql();
    if (!where.empty()) {
      sql += " WHERE " + where;
    }
  }
  sql += " ORDER BY effective_albumartist, album, disc, track, title";
  SqlQuery query(database_, sql);
  while (query.Step()) {
    songs.push_back(SongFromQuery(query));
  }
  return songs;
}

SongList CollectionBackend::Songs(const CollectionFilterOptions &options) const {
  SongList songs;
  if (!database_ || !database_->handle()) {
    return songs;
  }
  CollectionQuery query(database_, "songs", options);
  query.SetColumnSpec("songs.ROWID, " + std::string(Song::kColumnSpec));
  query.SetOrderBy("effective_albumartist, album, disc, track, title");
  if (!query.Exec()) {
    return songs;
  }
  while (query.Next()) {
    const Song song = SongFromQuery(*query.query());
    if (options.Matches(song)) {
      songs.push_back(song);
    }
  }
  return songs;
}

Song CollectionBackend::SongById(int id) const {
  SqlQuery query(database_, "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE ROWID = ?");
  query.Bind(1, id);
  if (query.Step()) {
    return SongFromQuery(query);
  }
  return Song();
}

Song CollectionBackend::SongByUrl(const std::string &url) const {
  SqlQuery query(database_, "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE url = ?");
  query.Bind(1, url);
  if (query.Step()) {
    return SongFromQuery(query);
  }
  return Song();
}

int CollectionBackend::AddOrUpdateSong(const Song &song) {
  const Song existing = SongByUrl(song.url());
  if (existing.is_valid()) {
    SqlQuery query(database_,
                   "UPDATE songs SET title=?, album=?, artist=?, albumartist=?, track=?, disc=?, year=?, genre=?, "
                   "composer=?, performer=?, grouping=?, comment=?, lyrics=?, length=?, bitrate=?, samplerate=?, "
                   "bitdepth=?, filetype=?, filesize=?, mtime=?, unavailable=0, art_embedded=?, "
                   "playcount=CASE WHEN ? > 0 THEN ? ELSE playcount END, "
                   "rating=CASE WHEN ? >= 0 THEN ? ELSE rating END WHERE url=?");
    query.Bind(1, song.title());
    query.Bind(2, song.album());
    query.Bind(3, song.artist());
    query.Bind(4, song.albumartist());
    query.Bind(5, song.track());
    query.Bind(6, song.disc());
    query.Bind(7, song.year());
    query.Bind(8, song.genre());
    query.Bind(9, song.composer());
    query.Bind(10, song.performer());
    query.Bind(11, song.grouping());
    query.Bind(12, song.comment());
    query.Bind(13, song.lyrics());
    query.Bind(14, song.length_nanosec());
    query.Bind(15, song.bitrate());
    query.Bind(16, song.samplerate());
    query.Bind(17, song.bitdepth());
    query.Bind(18, static_cast<int>(song.filetype()));
    query.Bind(19, song.filesize());
    query.Bind(20, song.mtime());
    query.Bind(21, song.art_embedded() ? 1 : 0);
    query.Bind(22, static_cast<int>(song.playcount()));
    query.Bind(23, static_cast<int>(song.playcount()));
    query.Bind(24, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
    query.Bind(25, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
    query.Bind(26, song.url());
    query.Exec();
    return existing.id();
  }

  SqlQuery query(database_,
                 "INSERT INTO songs (title, album, artist, albumartist, track, disc, year, genre, composer, performer, "
                 "grouping, comment, lyrics, beginning, length, bitrate, samplerate, bitdepth, source, directory_id, url, "
                 "filetype, filesize, mtime, ctime, unavailable, playcount, skipcount, lastplayed, lastseen, "
                 "compilation, compilation_detected, compilation_on, compilation_off, compilation_effective, "
                 "art_embedded, effective_albumartist, effective_originalyear, rating) "
                 "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,0,-1,-1,0,0,0,0,0,?,?,?,?)");
  query.Bind(1, song.title());
  query.Bind(2, song.album());
  query.Bind(3, song.artist());
  query.Bind(4, song.albumartist());
  query.Bind(5, song.track());
  query.Bind(6, song.disc());
  query.Bind(7, song.year());
  query.Bind(8, song.genre());
  query.Bind(9, song.composer());
  query.Bind(10, song.performer());
  query.Bind(11, song.grouping());
  query.Bind(12, song.comment());
  query.Bind(13, song.lyrics());
  query.Bind(14, song.beginning_nanosec());
  query.Bind(15, song.length_nanosec());
  query.Bind(16, song.bitrate());
  query.Bind(17, song.samplerate());
  query.Bind(18, song.bitdepth());
  query.Bind(19, static_cast<int>(song.source() == Song::Source::Unknown ? Song::Source::Collection : song.source()));
  query.Bind(20, song.directory_id());
  query.Bind(21, song.url());
  query.Bind(22, static_cast<int>(song.filetype()));
  query.Bind(23, song.filesize());
  query.Bind(24, song.mtime());
  query.Bind(25, song.ctime());
  query.Bind(26, static_cast<int>(song.playcount()));
  query.Bind(27, song.art_embedded() ? 1 : 0);
  query.Bind(28, song.EffectiveAlbumartist());
  query.Bind(29, song.originalyear() > 0 ? song.originalyear() : song.year());
  query.Bind(30, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
  query.Exec();
  return static_cast<int>(database_->LastInsertRowId());
}

void CollectionBackend::DeleteSongsInDirectory(int directory_id) {
  SqlQuery query(database_, "DELETE FROM songs WHERE directory_id = ?");
  query.Bind(1, directory_id);
  query.Exec();
}

int CollectionBackend::DeleteSongsBySource(Song::Source source) {
  if (!database_ || !database_->handle()) {
    return 0;
  }
  SongList deleted;
  for (const Song &song : Songs()) {
    if (song.source() == source) {
      deleted.push_back(song);
    }
  }
  if (deleted.empty()) {
    return 0;
  }
  SqlQuery query(database_, "DELETE FROM songs WHERE source = ?");
  query.Bind(1, static_cast<int>(source));
  query.Exec();
  SongsDeleted.Emit(deleted);
  return static_cast<int>(deleted.size());
}

void CollectionBackend::IncrementPlayCount(int song_id) {
  SqlQuery query(database_, "UPDATE songs SET playcount = playcount + 1, lastplayed = strftime('%s','now') WHERE ROWID = ?");
  query.Bind(1, song_id);
  query.Exec();
}

void CollectionBackend::IncrementSkipCount(int song_id) {
  SqlQuery query(database_, "UPDATE songs SET skipcount = skipcount + 1 WHERE ROWID = ?");
  query.Bind(1, song_id);
  query.Exec();
}

void CollectionBackend::ResetPlayStatistics(int song_id) {
  SqlQuery query(database_, "UPDATE songs SET playcount = 0, skipcount = 0, lastplayed = -1 WHERE ROWID = ?");
  query.Bind(1, song_id);
  query.Exec();
}

void CollectionBackend::SetRating(int song_id, float rating) {
  SqlQuery query(database_, "UPDATE songs SET rating = ? WHERE ROWID = ?");
  query.Bind(1, static_cast<int>(rating * 100.0f));
  query.Bind(2, song_id);
  query.Exec();
}

int CollectionBackend::ForceCompilation(const SongList &songs, bool on) {
  if (!database_ || !database_->handle()) {
    return 0;
  }
  int updated = 0;
  for (const auto &key : CollectionCompilation::AlbumArtistKeys(songs)) {
    SqlQuery query(database_,
                   "UPDATE songs SET compilation_on = ?, compilation_off = ?, "
                   "compilation_effective = ((compilation OR compilation_detected OR ?) AND NOT ?) + 0 "
                   "WHERE album = ? AND artist = ? AND unavailable = 0");
    query.Bind(1, on ? 1 : 0);
    query.Bind(2, on ? 0 : 1);
    query.Bind(3, on ? 1 : 0);
    query.Bind(4, on ? 0 : 1);
    query.Bind(5, key.first);
    query.Bind(6, key.second);
    if (query.Exec()) {
      ++updated;
    }
  }
  return updated;
}

void CollectionBackend::SetUnavailable(int song_id, bool unavailable) {
  SqlQuery query(database_, "UPDATE songs SET unavailable = ? WHERE ROWID = ?");
  query.Bind(1, unavailable ? 1 : 0);
  query.Bind(2, song_id);
  query.Exec();
}

int CollectionBackend::MarkMissingUnavailable(int directory_id, const std::vector<std::string> &seen_urls) {
  SongList songs;
  SqlQuery query(database_, "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE directory_id = ?");
  query.Bind(1, directory_id);
  while (query.Step()) {
    songs.push_back(SongFromQuery(query));
  }
  int marked = 0;
  for (const Song &song : songs) {
    if (song.unavailable()) {
      continue;
    }
    if (std::find(seen_urls.begin(), seen_urls.end(), song.url()) == seen_urls.end()) {
      SetUnavailable(song.id(), true);
      ++marked;
    }
  }
  return marked;
}

int CollectionBackend::SongCount() const {
  SqlQuery query(database_, "SELECT COUNT(*) FROM songs WHERE unavailable = 0");
  if (query.Step()) {
    return query.ColumnInt(0);
  }
  return 0;
}
