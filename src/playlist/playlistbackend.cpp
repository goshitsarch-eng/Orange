#include "playlist/playlistbackend.h"

#include "collection/collectionbackend.h"
#include "core/database.h"
#include "tagreader/tagreader.h"

PlaylistBackend::PlaylistBackend(Database *database, TagReader *tagreader, CollectionBackend *collection_backend)
    : database_(database), tagreader_(tagreader), collection_backend_(collection_backend) {}

std::vector<PlaylistMetadata> PlaylistBackend::GetAllPlaylists() {
  std::vector<PlaylistMetadata> result;
  SqlQuery query(database_,
                 "SELECT ROWID, name, last_played, ui_order, special_type, ui_path, is_favorite FROM playlists ORDER BY ui_order, name");
  while (query.Step()) {
    PlaylistMetadata metadata;
    metadata.id = query.ColumnInt(0);
    metadata.name = query.ColumnText(1);
    metadata.last_played = query.ColumnInt(2);
    metadata.ui_order = query.ColumnInt(3);
    metadata.special_type = query.ColumnText(4);
    metadata.ui_path = query.ColumnText(5);
    metadata.favorite = query.ColumnInt(6) != 0;
    result.push_back(metadata);
  }
  return result;
}

std::unique_ptr<Playlist> PlaylistBackend::LoadPlaylist(int id) {
  auto playlist = std::make_unique<Playlist>();
  playlist->set_id(id);
  SqlQuery meta(database_, "SELECT name, is_favorite, last_played, ui_path FROM playlists WHERE ROWID = ?");
  meta.Bind(1, id);
  int last_played = -1;
  if (meta.Step()) {
    playlist->set_name(meta.ColumnText(0));
    playlist->set_favorite(meta.ColumnInt(1) != 0);
    last_played = meta.ColumnInt(2);
    playlist->set_ui_path(meta.ColumnText(3));
  }
  SqlQuery items(database_, "SELECT collection_id, url, title, album, artist, albumartist, track, length FROM playlist_items WHERE playlist = ?");
  items.Bind(1, id);
  SongList songs;
  while (items.Step()) {
    const int collection_id = items.ColumnInt(0);
    Song song;
    if (collection_id > 0 && collection_backend_) {
      song = collection_backend_->SongById(collection_id);
    }
    if (!song.is_valid()) {
      song.set_url(items.ColumnText(1));
      song.set_title(items.ColumnText(2));
      song.set_album(items.ColumnText(3));
      song.set_artist(items.ColumnText(4));
      song.set_albumartist(items.ColumnText(5));
      song.set_track(items.ColumnInt(6));
      song.set_length_nanosec(items.ColumnInt64(7));
      song.set_valid(!song.url().empty());
    }
    songs.push_back(song);
  }
  playlist->BeginLoad();
  playlist->AppendSongs(songs);
  playlist->EndLoad();
  if (last_played >= 0 && last_played < playlist->row_count()) {
    playlist->set_current_row(last_played);
  }
  return playlist;
}

int PlaylistBackend::SavePlaylist(Playlist *playlist) {
  if (!playlist) {
    return -1;
  }
  if (playlist->id() < 0) {
    SqlQuery query(database_, "INSERT INTO playlists (name, last_played, ui_order, is_favorite, ui_path) VALUES (?, ?, 0, ?, ?)");
    query.Bind(1, playlist->name());
    query.Bind(2, playlist->current_row());
    query.Bind(3, playlist->favorite() ? 1 : 0);
    query.Bind(4, playlist->ui_path());
    query.Exec();
    playlist->set_id(static_cast<int>(database_->LastInsertRowId()));
  } else {
    SqlQuery query(database_, "UPDATE playlists SET name = ?, is_favorite = ?, last_played = ?, ui_path = ? WHERE ROWID = ?");
    query.Bind(1, playlist->name());
    query.Bind(2, playlist->favorite() ? 1 : 0);
    query.Bind(3, playlist->current_row());
    query.Bind(4, playlist->ui_path());
    query.Bind(5, playlist->id());
    query.Exec();
    SqlQuery clear(database_, "DELETE FROM playlist_items WHERE playlist = ?");
    clear.Bind(1, playlist->id());
    clear.Exec();
  }
  for (const Song &song : playlist->songs()) {
    SqlQuery item(database_,
                  "INSERT INTO playlist_items (playlist, type, collection_id, url, title, album, artist, albumartist, track, length, source) "
                  "VALUES (?, 0, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    item.Bind(1, playlist->id());
    item.Bind(2, song.id());
    item.Bind(3, song.url());
    item.Bind(4, song.title());
    item.Bind(5, song.album());
    item.Bind(6, song.artist());
    item.Bind(7, song.albumartist());
    item.Bind(8, song.track());
    item.Bind(9, song.length_nanosec());
    item.Bind(10, static_cast<int>(song.source()));
    item.Exec();
  }
  return playlist->id();
}

void PlaylistBackend::DeletePlaylist(int id) {
  SqlQuery items(database_, "DELETE FROM playlist_items WHERE playlist = ?");
  items.Bind(1, id);
  items.Exec();
  SqlQuery playlist(database_, "DELETE FROM playlists WHERE ROWID = ?");
  playlist.Bind(1, id);
  playlist.Exec();
}

void PlaylistBackend::RenamePlaylist(int id, const std::string &name) {
  SqlQuery query(database_, "UPDATE playlists SET name = ? WHERE ROWID = ?");
  query.Bind(1, name);
  query.Bind(2, id);
  query.Exec();
}

void PlaylistBackend::SetFavorite(int id, bool favorite) {
  SqlQuery query(database_, "UPDATE playlists SET is_favorite = ? WHERE ROWID = ?");
  query.Bind(1, favorite ? 1 : 0);
  query.Bind(2, id);
  query.Exec();
}

void PlaylistBackend::SetPlaylistUiPath(int id, const std::string &path) {
  SqlQuery query(database_, "UPDATE playlists SET ui_path = ? WHERE ROWID = ?");
  query.Bind(1, path);
  query.Bind(2, id);
  query.Exec();
}

void PlaylistBackend::SaveLastPlayed(int id, int row) {
  SqlQuery query(database_, "UPDATE playlists SET last_played = ? WHERE ROWID = ?");
  query.Bind(1, row);
  query.Bind(2, id);
  query.Exec();
}
