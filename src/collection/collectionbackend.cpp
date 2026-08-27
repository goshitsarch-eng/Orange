#include "collection/collectionbackend.h"

#include "collection/collectionalbumart.h"
#include "collection/collectionartpersist.h"
#include "collection/collectioncompilation.h"
#include "collection/collectioncompilationdetect.h"
#include "collection/collectionexpire.h"
#include "collection/collectionfingerprintmatch.h"
#include "collection/collectionquery.h"
#include "filterparser/filterparser.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <ctime>

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
  SqlQuery subdirs(database_, "DELETE FROM subdirectories WHERE directory_id = ?");
  subdirs.Bind(1, id);
  subdirs.Exec();
  SqlQuery query(database_, "DELETE FROM directories WHERE rowid = ?");
  query.Bind(1, id);
  query.Exec();
  DirectoryDeleted.Emit();
}

std::vector<CollectionSubdirectory> CollectionBackend::SubdirsInDirectory(int directory_id) const {
  std::vector<CollectionSubdirectory> result;
  if (!database_ || !database_->handle()) {
    return result;
  }
  SqlQuery query(database_, "SELECT directory_id, path, mtime FROM subdirectories WHERE directory_id = ? ORDER BY path");
  query.Bind(1, directory_id);
  while (query.Step()) {
    CollectionSubdirectory subdir;
    subdir.directory_id = query.ColumnInt(0);
    subdir.path = query.ColumnText(1);
    subdir.mtime = query.ColumnInt64(2);
    result.push_back(subdir);
  }
  return result;
}

void CollectionBackend::AddOrUpdateSubdirs(int directory_id, const std::vector<CollectionSubdirectory> &subdirs) {
  if (!database_ || !database_->handle()) {
    return;
  }
  SqlQuery clear(database_, "DELETE FROM subdirectories WHERE directory_id = ?");
  clear.Bind(1, directory_id);
  clear.Exec();
  for (const CollectionSubdirectory &subdir : subdirs) {
    SqlQuery insert(database_, "INSERT INTO subdirectories (directory_id, path, mtime) VALUES (?, ?, ?)");
    insert.Bind(1, directory_id);
    insert.Bind(2, subdir.path);
    insert.Bind(3, subdir.mtime);
    insert.Exec();
  }
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
  song.set_compilation_on(query.ColumnInt(44) != 0);
  song.set_compilation_off(query.ColumnInt(45) != 0);
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
  song.set_fingerprint(query.ColumnText(38));
  song.set_playcount(static_cast<unsigned>(query.ColumnInt(39)));
  song.set_skipcount(static_cast<unsigned>(query.ColumnInt(40)));
  song.set_lastplayed(query.ColumnInt64(41));
  song.set_lastseen(query.ColumnInt64(42));
  song.set_art_embedded(query.ColumnInt(47) != 0);
  song.set_art_automatic(query.ColumnText(48));
  song.set_art_manual(query.ColumnText(49));
  song.set_art_unset(query.ColumnInt(50) != 0);
  song.set_cue_path(query.ColumnText(53));
  song.set_rating(static_cast<float>(query.ColumnInt(54)) / 100.0f);
  if (!query.ColumnIsNull(67)) {
    song.set_ebur128_integrated_loudness_lufs(query.ColumnDouble(67));
  }
  if (!query.ColumnIsNull(68)) {
    song.set_ebur128_loudness_range_lu(query.ColumnDouble(68));
  }
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

Song CollectionBackend::SongByUrl(const std::string &url, int64_t beginning_nanosec) const {
  std::string sql = "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE url = ?";
  if (beginning_nanosec >= 0) {
    sql += " AND beginning = ?";
  }
  SqlQuery query(database_, sql);
  query.Bind(1, url);
  if (beginning_nanosec >= 0) {
    query.Bind(2, beginning_nanosec);
  }
  if (query.Step()) {
    return SongFromQuery(query);
  }
  return Song();
}

SongList CollectionBackend::SongsByFingerprint(const std::string &fingerprint) const {
  SongList songs;
  if (!CollectionFingerprintMatch::IsUsable(fingerprint) || !database_ || !database_->handle()) {
    return songs;
  }
  SqlQuery query(database_, "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE fingerprint = ?");
  query.Bind(1, fingerprint);
  while (query.Step()) {
    songs.push_back(SongFromQuery(query));
  }
  return songs;
}

void CollectionBackend::UpdateCompilations() {
  if (!database_ || !database_->handle()) {
    return;
  }
  SqlQuery query(database_,
                 "SELECT directory_id, album, COUNT(DISTINCT CASE WHEN albumartist != '' THEN albumartist ELSE artist END) "
                 "FROM songs WHERE unavailable = 0 AND album != '' GROUP BY directory_id, album");
  while (query.Step()) {
    const int directory_id = query.ColumnInt(0);
    const std::string album = query.ColumnText(1);
    const bool detected = CollectionCompilationDetect::IsCompilationAlbum(query.ColumnInt(2));
    SqlQuery update(database_,
                    "UPDATE songs SET compilation_detected = ?, "
                    "compilation_effective = ((compilation OR ? OR compilation_on) AND NOT compilation_off) + 0 "
                    "WHERE directory_id = ? AND album = ? AND unavailable = 0");
    update.Bind(1, detected ? 1 : 0);
    update.Bind(2, detected ? 1 : 0);
    update.Bind(3, directory_id);
    update.Bind(4, album);
    update.Exec();
  }
}

void CollectionBackend::UpdateSongUrl(int song_id, const std::string &url, int directory_id) {
  if (song_id <= 0 || url.empty() || !database_ || !database_->handle()) {
    return;
  }
  if (directory_id >= 0) {
    SqlQuery query(database_, "UPDATE songs SET url = ?, directory_id = ? WHERE ROWID = ?");
    query.Bind(1, url);
    query.Bind(2, directory_id);
    query.Bind(3, song_id);
    query.Exec();
  } else {
    SqlQuery query(database_, "UPDATE songs SET url = ? WHERE ROWID = ?");
    query.Bind(1, url);
    query.Bind(2, song_id);
    query.Exec();
  }
  const Song song = SongById(song_id);
  if (song.id() > 0) {
    SongsStatisticsChanged.Emit({song});
  }
}

int CollectionBackend::AddOrUpdateSong(const Song &song) {
  Song existing = SongByUrl(song.url(), song.beginning_nanosec());
  if (!existing.is_valid() && song.id() > 0) {
    existing = SongById(song.id());
  }
  if (!existing.is_valid() && CollectionFingerprintMatch::IsUsable(song.fingerprint())) {
    const Song moved = CollectionFingerprintMatch::PickMovedMatch(SongsByFingerprint(song.fingerprint()), song.url());
    if (moved.is_valid()) {
      existing = moved;
    }
  }
  if (existing.is_valid()) {
    SqlQuery query(database_,
                   "UPDATE songs SET title=?, album=?, artist=?, albumartist=?, track=?, disc=?, year=?, genre=?, "
                   "composer=?, performer=?, grouping=?, comment=?, lyrics=?, beginning=?, length=?, bitrate=?, samplerate=?, "
                   "bitdepth=?, filetype=?, filesize=?, mtime=?, unavailable=0, art_embedded=?, art_manual=?, fingerprint=?, "
                   "cue_path=?, skipcount=?, lastplayed=?, ebur128_integrated_loudness_lufs=?, ebur128_loudness_range_lu=?, "
                   "playcount=CASE WHEN ? > 0 THEN ? ELSE playcount END, "
                   "rating=CASE WHEN ? >= 0 THEN ? ELSE rating END, art_automatic=?, url=?, compilation_on=?, "
                   "compilation_off=?, art_unset=? WHERE ROWID=?");
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
    query.Bind(19, static_cast<int>(song.filetype()));
    query.Bind(20, song.filesize());
    query.Bind(21, song.mtime());
    query.Bind(22, song.art_embedded() ? 1 : 0);
    query.Bind(23, song.art_manual());
    query.Bind(24, song.fingerprint());
    query.Bind(25, song.cue_path());
    query.Bind(26, static_cast<int>(song.skipcount()));
    query.Bind(27, song.lastplayed());
    if (song.ebur128_integrated_loudness_lufs()) {
      query.Bind(28, *song.ebur128_integrated_loudness_lufs());
    } else {
      query.BindNull(28);
    }
    if (song.ebur128_loudness_range_lu()) {
      query.Bind(29, *song.ebur128_loudness_range_lu());
    } else {
      query.BindNull(29);
    }
    query.Bind(30, static_cast<int>(song.playcount()));
    query.Bind(31, static_cast<int>(song.playcount()));
    query.Bind(32, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
    query.Bind(33, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
    const bool compilation_on = song.compilation_on() || (!song.compilation_off() && existing.compilation_on());
    const bool compilation_off = song.compilation_off() || (!song.compilation_on() && existing.compilation_off());
    query.Bind(34, CollectionArtPersist::ArtAutomaticForUpdate(existing, song.art_automatic()));
    query.Bind(35, song.url());
    query.Bind(36, compilation_on ? 1 : 0);
    query.Bind(37, compilation_off ? 1 : 0);
    query.Bind(38, (song.art_unset() || existing.art_unset()) ? 1 : 0);
    query.Bind(39, existing.id());
    query.Exec();
    return existing.id();
  }

  SqlQuery query(database_,
                 "INSERT INTO songs (title, album, artist, albumartist, track, disc, year, genre, composer, performer, "
                 "grouping, comment, lyrics, beginning, length, bitrate, samplerate, bitdepth, source, directory_id, url, "
                 "filetype, filesize, mtime, ctime, unavailable, fingerprint, playcount, skipcount, lastplayed, lastseen, "
                 "compilation, compilation_detected, compilation_on, compilation_off, compilation_effective, "
                 "art_embedded, art_automatic, art_manual, effective_albumartist, effective_originalyear, cue_path, rating, "
                 "ebur128_integrated_loudness_lufs, ebur128_loudness_range_lu) "
                 "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,?,?,?,?,-1,0,0,0,0,0,?,?,?,?,?,?,?,?,?)");
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
  query.Bind(26, song.fingerprint());
  query.Bind(27, static_cast<int>(song.playcount()));
  query.Bind(28, static_cast<int>(song.skipcount()));
  query.Bind(29, song.lastplayed());
  query.Bind(30, song.art_embedded() ? 1 : 0);
  query.Bind(31, song.art_automatic());
  query.Bind(32, song.art_manual());
  query.Bind(33, song.EffectiveAlbumartist());
  query.Bind(34, song.originalyear() > 0 ? song.originalyear() : song.year());
  query.Bind(35, song.cue_path());
  query.Bind(36, song.rating() >= 0 ? static_cast<int>(song.rating() * 100.0f) : -1);
  if (song.ebur128_integrated_loudness_lufs()) {
    query.Bind(37, *song.ebur128_integrated_loudness_lufs());
  } else {
    query.BindNull(37);
  }
  if (song.ebur128_loudness_range_lu()) {
    query.Bind(38, *song.ebur128_loudness_range_lu());
  } else {
    query.BindNull(38);
  }
  query.Exec();
  return static_cast<int>(database_->LastInsertRowId());
}

int CollectionBackend::RetainBeginnings(const std::string &url, const std::vector<int64_t> &beginnings) {
  if (url.empty() || !database_ || !database_->handle() || beginnings.empty()) {
    return 0;
  }
  SongList keep_check;
  SqlQuery query(database_, "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs WHERE url = ?");
  query.Bind(1, url);
  int removed = 0;
  while (query.Step()) {
    const Song song = SongFromQuery(query);
    if (std::find(beginnings.begin(), beginnings.end(), song.beginning_nanosec()) == beginnings.end()) {
      SqlQuery del(database_, "DELETE FROM songs WHERE ROWID = ?");
      del.Bind(1, song.id());
      del.Exec();
      ++removed;
    }
  }
  return removed;
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
  const Song song = SongById(song_id);
  if (song.id() > 0) {
    SongsStatisticsChanged.Emit({song});
  }
}

void CollectionBackend::IncrementSkipCount(int song_id) {
  SqlQuery query(database_, "UPDATE songs SET skipcount = skipcount + 1 WHERE ROWID = ?");
  query.Bind(1, song_id);
  query.Exec();
  const Song song = SongById(song_id);
  if (song.id() > 0) {
    SongsStatisticsChanged.Emit({song});
  }
}

void CollectionBackend::ResetPlayStatistics(int song_id) {
  SqlQuery query(database_, "UPDATE songs SET playcount = 0, skipcount = 0, lastplayed = -1 WHERE ROWID = ?");
  query.Bind(1, song_id);
  query.Exec();
  const Song song = SongById(song_id);
  if (song.id() > 0) {
    SongsStatisticsChanged.Emit({song});
  }
}

void CollectionBackend::SetRating(int song_id, float rating) {
  SqlQuery query(database_, "UPDATE songs SET rating = ? WHERE ROWID = ?");
  query.Bind(1, static_cast<int>(rating * 100.0f));
  query.Bind(2, song_id);
  query.Exec();
  const Song song = SongById(song_id);
  if (song.id() > 0) {
    SongsRatingChanged.Emit({song});
  }
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

void CollectionBackend::UpdateLastSeen(int directory_id) {
  if (!database_ || !database_->handle() || directory_id < 0) {
    return;
  }
  SqlQuery query(database_, "UPDATE songs SET lastseen = ? WHERE directory_id = ? AND unavailable = 0");
  query.Bind(1, static_cast<int64_t>(std::time(nullptr)));
  query.Bind(2, directory_id);
  query.Exec();
}

int CollectionBackend::ExpireSongs(int directory_id, int expire_days, int64_t now_sec) {
  if (!database_ || !database_->handle() || directory_id < 0 || expire_days <= 0) {
    return 0;
  }
  if (now_sec <= 0) {
    now_sec = static_cast<int64_t>(std::time(nullptr));
  }
  const int64_t cutoff = CollectionExpire::Cutoff(now_sec, expire_days);
  SqlQuery query(database_,
                 "SELECT songs.ROWID FROM songs LEFT JOIN playlist_items ON songs.ROWID = playlist_items.collection_id "
                 "WHERE songs.directory_id = ? AND songs.unavailable = 1 AND songs.lastseen > 0 AND songs.lastseen < ? "
                 "AND playlist_items.collection_id IS NULL");
  query.Bind(1, directory_id);
  query.Bind(2, cutoff);
  SongList expired;
  while (query.Step()) {
    const Song song = SongById(query.ColumnInt(0));
    if (song.is_valid()) {
      expired.push_back(song);
    }
  }
  int removed = 0;
  for (const Song &song : expired) {
    if (!CollectionExpire::ShouldExpire(song.lastseen(), cutoff, song.unavailable(), false)) {
      continue;
    }
    SqlQuery del(database_, "DELETE FROM songs WHERE ROWID = ?");
    del.Bind(1, song.id());
    del.Exec();
    ++removed;
  }
  if (!expired.empty()) {
    SongsDeleted.Emit(expired);
  }
  return removed;
}

SongList CollectionBackend::GetAlbumSongs(const std::string &effective_albumartist, const std::string &album) const {
  SongList songs;
  if (!CollectionAlbumArt::AlbumKeyValid(effective_albumartist, album) || !database_ || !database_->handle()) {
    return songs;
  }
  SqlQuery query(database_,
                 "SELECT ROWID, " + std::string(Song::kColumnSpec) +
                     " FROM songs WHERE effective_albumartist = ? AND album = ? AND unavailable = 0");
  query.Bind(1, effective_albumartist);
  query.Bind(2, album);
  while (query.Step()) {
    songs.push_back(SongFromQuery(query));
  }
  return songs;
}

int CollectionBackend::UpdateManualAlbumArt(const std::string &effective_albumartist, const std::string &album,
                                            const std::string &art_manual) {
  if (!CollectionAlbumArt::AlbumKeyValid(effective_albumartist, album) || !database_ || !database_->handle()) {
    return 0;
  }
  SqlQuery query(database_,
                 "UPDATE songs SET art_manual = ?, art_unset = 0 WHERE effective_albumartist = ? AND album = ? AND unavailable = 0");
  query.Bind(1, art_manual);
  query.Bind(2, effective_albumartist);
  query.Bind(3, album);
  query.Exec();
  const SongList songs = GetAlbumSongs(effective_albumartist, album);
  if (!songs.empty()) {
    SongsDiscovered.Emit(songs);
  }
  return static_cast<int>(songs.size());
}

int CollectionBackend::UpdateEmbeddedAlbumArt(const std::string &effective_albumartist, const std::string &album, bool embedded) {
  if (!CollectionAlbumArt::AlbumKeyValid(effective_albumartist, album) || !database_ || !database_->handle()) {
    return 0;
  }
  SqlQuery query(database_,
                 "UPDATE songs SET art_embedded = ?, art_unset = 0 WHERE effective_albumartist = ? AND album = ? AND unavailable = 0");
  query.Bind(1, embedded ? 1 : 0);
  query.Bind(2, effective_albumartist);
  query.Bind(3, album);
  query.Exec();
  return static_cast<int>(GetAlbumSongs(effective_albumartist, album).size());
}

int CollectionBackend::ClearAlbumArt(const std::string &effective_albumartist, const std::string &album, bool art_unset) {
  if (!CollectionAlbumArt::AlbumKeyValid(effective_albumartist, album) || !database_ || !database_->handle()) {
    return 0;
  }
  SqlQuery query(database_,
                 "UPDATE songs SET art_embedded = 0, art_automatic = '', art_manual = '', art_unset = ? "
                 "WHERE effective_albumartist = ? AND album = ? AND unavailable = 0");
  query.Bind(1, art_unset ? 1 : 0);
  query.Bind(2, effective_albumartist);
  query.Bind(3, album);
  query.Exec();
  return static_cast<int>(GetAlbumSongs(effective_albumartist, album).size());
}

int CollectionBackend::UnsetAlbumArt(const std::string &effective_albumartist, const std::string &album) {
  return ClearAlbumArt(effective_albumartist, album, true);
}

int CollectionBackend::SongCount() const {
  SqlQuery query(database_, "SELECT COUNT(*) FROM songs WHERE unavailable = 0");
  if (query.Step()) {
    return query.ColumnInt(0);
  }
  return 0;
}
