#include "streaming/streamingcollectionstore.h"

#include "core/database.h"
#include "core/scopedtransaction.h"

namespace {

Song SongFromRow(const SqlQuery &query) {
  Song song(static_cast<Song::Source>(query.ColumnInt(15)));
  song.set_title(query.ColumnText(0));
  song.set_album(query.ColumnText(1));
  song.set_artist(query.ColumnText(2));
  song.set_albumartist(query.ColumnText(3));
  song.set_artist_id(query.ColumnText(4));
  song.set_album_id(query.ColumnText(5));
  song.set_song_id(query.ColumnText(6));
  song.set_url(query.ColumnText(7));
  song.set_art_automatic(query.ColumnText(8));
  song.set_year(query.ColumnInt(9));
  song.set_genre(query.ColumnText(10));
  song.set_track(query.ColumnInt(11));
  song.set_disc(query.ColumnInt(12));
  song.set_length_nanosec(query.ColumnInt64(13));
  song.set_comment(query.ColumnText(14));
  song.set_valid(true);
  return song;
}

void InsertSong(Database *database, const std::string &table, const Song &song) {
  const std::string url = StreamingCollectionStore::PersistUrl(song);
  if (!database || url.empty()) {
    return;
  }
  SqlQuery query(database, "INSERT INTO " + table +
                               " (title, album, artist, albumartist, artist_id, album_id, song_id, url, art_automatic, year, genre, "
                               "track, disc, length, comment, source) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
  query.Bind(1, song.title());
  query.Bind(2, song.album());
  query.Bind(3, song.artist());
  query.Bind(4, song.albumartist());
  query.Bind(5, song.artist_id());
  query.Bind(6, song.album_id());
  query.Bind(7, song.song_id());
  query.Bind(8, url);
  query.Bind(9, song.art_automatic());
  query.Bind(10, song.year());
  query.Bind(11, song.genre());
  query.Bind(12, song.track());
  query.Bind(13, song.disc());
  query.Bind(14, song.length_nanosec());
  query.Bind(15, song.comment());
  query.Bind(16, static_cast<int>(song.source()));
  query.Exec();
}

}  // namespace

namespace StreamingCollectionStore {

SongList Load(Database *database, const std::string &table) {
  SongList songs;
  if (!database || !database->handle() || !ValidTable(table)) {
    return songs;
  }
  SqlQuery query(database, "SELECT title, album, artist, albumartist, artist_id, album_id, song_id, url, art_automatic, year, genre, "
                           "track, disc, length, comment, source FROM " +
                               table + " ORDER BY artist, album, disc, track, title");
  while (query.Step()) {
    songs.push_back(SongFromRow(query));
  }
  return songs;
}

void Replace(Database *database, const std::string &table, const SongList &songs) {
  if (!database || !database->handle() || !ValidTable(table)) {
    return;
  }
  ScopedTransaction transaction(database);
  database->Exec("DELETE FROM " + table);
  for (const Song &song : songs) {
    InsertSong(database, table, song);
  }
  transaction.Commit();
}

}  // namespace StreamingCollectionStore
