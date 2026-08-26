#include "collection/collectionbackend.h"

#include "filterparser/filterparser.h"
#include "utilities/strutils.h"

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
  song.set_genre(query.ColumnText(13));
  song.set_composer(query.ColumnText(15));
  song.set_performer(query.ColumnText(17));
  song.set_grouping(query.ColumnText(19));
  song.set_comment(query.ColumnText(20));
  song.set_lyrics(query.ColumnText(21));
  song.set_length_nanosec(query.ColumnInt64(25));
  song.set_bitrate(query.ColumnInt(26));
  song.set_samplerate(query.ColumnInt(27));
  song.set_bitdepth(query.ColumnInt(28));
  song.set_source(static_cast<Song::Source>(query.ColumnInt(29)));
  song.set_directory_id(query.ColumnInt(30));
  song.set_url(query.ColumnText(31));
  song.set_filetype(static_cast<Song::FileType>(query.ColumnInt(32)));
  song.set_filesize(query.ColumnInt64(33));
  song.set_playcount(static_cast<unsigned>(query.ColumnInt(38)));
  song.set_skipcount(static_cast<unsigned>(query.ColumnInt(39)));
  song.set_rating(static_cast<float>(query.ColumnInt(53)) / 100.0f);
  song.set_valid(true);
  return song;
}

SongList CollectionBackend::Songs(const std::string &filter) const {
  SongList songs;
  const bool advanced = !filter.empty() && (filter.find(':') != std::string::npos || filter[0] == '-');
  std::string sql = "SELECT ROWID, " + std::string(Song::kColumnSpec) + " FROM songs";
  if (!filter.empty() && !advanced) {
    sql += " WHERE title LIKE ? OR album LIKE ? OR artist LIKE ? OR albumartist LIKE ? OR composer LIKE ? OR genre LIKE ?";
  }
  sql += " ORDER BY effective_albumartist, album, disc, track, title";
  SqlQuery query(database_, sql);
  if (!filter.empty() && !advanced) {
    const std::string like = "%" + StrUtils::SqlLikeEscape(filter) + "%";
    for (int i = 1; i <= 6; ++i) {
      query.Bind(i, like);
    }
  }
  while (query.Step()) {
    songs.push_back(SongFromQuery(query));
  }
  if (advanced) {
    FilterParser parser(filter);
    SongList filtered;
    for (const Song &song : songs) {
      if (parser.Matches(song)) {
        filtered.push_back(song);
      }
    }
    return filtered;
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
                   "bitdepth=?, filetype=?, filesize=?, mtime=?, unavailable=0 WHERE url=?");
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
    query.Bind(21, song.url());
    query.Exec();
    return existing.id();
  }

  SqlQuery query(database_,
                 "INSERT INTO songs (title, album, artist, albumartist, track, disc, year, genre, composer, performer, "
                 "grouping, comment, lyrics, beginning, length, bitrate, samplerate, bitdepth, source, directory_id, url, "
                 "filetype, filesize, mtime, ctime, unavailable, playcount, skipcount, lastplayed, lastseen, "
                 "compilation, compilation_detected, compilation_on, compilation_off, compilation_effective, "
                 "effective_albumartist, effective_originalyear, rating) "
                 "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,0,0,-1,-1,0,0,0,0,0,?,?, -1)");
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
  query.Bind(26, song.EffectiveAlbumartist());
  query.Bind(27, song.originalyear() > 0 ? song.originalyear() : song.year());
  query.Exec();
  return static_cast<int>(database_->LastInsertRowId());
}

void CollectionBackend::DeleteSongsInDirectory(int directory_id) {
  SqlQuery query(database_, "DELETE FROM songs WHERE directory_id = ?");
  query.Bind(1, directory_id);
  query.Exec();
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

void CollectionBackend::SetRating(int song_id, float rating) {
  SqlQuery query(database_, "UPDATE songs SET rating = ? WHERE ROWID = ?");
  query.Bind(1, static_cast<int>(rating * 100.0f));
  query.Bind(2, song_id);
  query.Exec();
}

int CollectionBackend::SongCount() const {
  SqlQuery query(database_, "SELECT COUNT(*) FROM songs WHERE unavailable = 0");
  if (query.Step()) {
    return query.ColumnInt(0);
  }
  return 0;
}
