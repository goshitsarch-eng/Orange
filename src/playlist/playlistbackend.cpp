#include "playlist/playlistbackend.h"

#include "collection/collectionbackend.h"
#include "core/database.h"
#include "playlist/playlistdynamicpersist.h"
#include "playlist/playlistitembind.h"
#include "playlist/playlistitemuuid.h"
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
  SqlQuery meta(database_,
                "SELECT name, is_favorite, last_played, ui_path, dynamic_playlist_type, dynamic_playlist_data FROM playlists WHERE ROWID = ?");
  meta.Bind(1, id);
  int last_played = -1;
  int dynamic_type = 0;
  std::string dynamic_data;
  if (meta.Step()) {
    playlist->set_name(meta.ColumnText(0));
    playlist->set_favorite(meta.ColumnInt(1) != 0);
    last_played = meta.ColumnInt(2);
    playlist->set_ui_path(meta.ColumnText(3));
    dynamic_type = meta.ColumnInt(4);
    dynamic_data = meta.ColumnText(5);
  }
  SqlQuery items(database_, PlaylistItemBind::LoadSql());
  items.Bind(1, id);
  SongList songs;
  std::vector<std::string> uuids;
  while (items.Step()) {
    const int collection_id = items.ColumnInt(0);
    const std::string uuid = items.ColumnText(1);
    Song song;
    if (collection_id > 0 && collection_backend_) {
      song = collection_backend_->SongById(collection_id);
    }
    if (!song.is_valid()) {
      song.set_url(items.ColumnText(2));
      song.set_title(items.ColumnText(3));
      song.set_album(items.ColumnText(4));
      song.set_artist(items.ColumnText(5));
      song.set_albumartist(items.ColumnText(6));
      song.set_track(items.ColumnInt(7));
      song.set_disc(items.ColumnInt(8));
      song.set_year(items.ColumnInt(9));
      song.set_originalyear(items.ColumnInt(10));
      song.set_genre(items.ColumnText(11));
      song.set_composer(items.ColumnText(12));
      song.set_comment(items.ColumnText(13));
      song.set_lyrics(items.ColumnText(14));
      song.set_length_nanosec(items.ColumnInt64(15));
      const int rating = items.ColumnInt(16);
      if (rating >= 0) {
        song.set_rating(static_cast<float>(rating) / 100.0f);
      }
      song.set_cue_path(items.ColumnText(17));
      song.set_bpm(static_cast<float>(items.ColumnDouble(18)));
      song.set_mood(items.ColumnText(19));
      song.set_initial_key(items.ColumnText(20));
      song.set_source(static_cast<Song::Source>(items.ColumnInt(21)));
      song.set_valid(!song.url().empty());
    }
    songs.push_back(song);
    uuids.push_back(PlaylistItemUuid::Valid(uuid) ? uuid : PlaylistItemUuid::New());
  }
  playlist->BeginLoad();
  playlist->AppendSongs(songs);
  playlist->SetRowUuids(uuids);
  playlist->EndLoad();
  if (DynamicPlaylistPersist::IsDynamic(dynamic_type)) {
    playlist->SetDynamic(true, DynamicPlaylistPersist::Decode(dynamic_data));
    if (collection_backend_ && playlist->dynamic_generator()) {
      playlist->dynamic_generator()->set_collection_backend(collection_backend_);
    }
  }
  playlist->set_last_played_row(last_played);
  return playlist;
}

int PlaylistBackend::SavePlaylist(Playlist *playlist) {
  if (!playlist) {
    return -1;
  }
  playlist->EnsureUuids();
  if (playlist->id() < 0) {
    SqlQuery query(database_,
                   "INSERT INTO playlists (name, last_played, ui_order, is_favorite, ui_path, dynamic_playlist_type, "
                   "dynamic_playlist_backend, dynamic_playlist_data) VALUES (?, ?, 0, ?, ?, ?, ?, ?)");
    query.Bind(1, playlist->name());
    query.Bind(2, playlist->last_played_row());
    query.Bind(3, playlist->favorite() ? 1 : 0);
    query.Bind(4, playlist->ui_path());
    query.Bind(5, DynamicPlaylistPersist::TypeFor(playlist->is_dynamic()));
    query.Bind(6, DynamicPlaylistPersist::DefaultBackend());
    query.Bind(7, playlist->is_dynamic() ? DynamicPlaylistPersist::Encode(playlist->dynamic_search()) : std::string());
    query.Exec();
    playlist->set_id(static_cast<int>(database_->LastInsertRowId()));
  } else {
    SqlQuery query(database_,
                   "UPDATE playlists SET name = ?, is_favorite = ?, last_played = ?, ui_path = ?, dynamic_playlist_type = ?, "
                   "dynamic_playlist_backend = ?, dynamic_playlist_data = ? WHERE ROWID = ?");
    query.Bind(1, playlist->name());
    query.Bind(2, playlist->favorite() ? 1 : 0);
    query.Bind(3, playlist->last_played_row());
    query.Bind(4, playlist->ui_path());
    query.Bind(5, DynamicPlaylistPersist::TypeFor(playlist->is_dynamic()));
    query.Bind(6, DynamicPlaylistPersist::DefaultBackend());
    query.Bind(7, playlist->is_dynamic() ? DynamicPlaylistPersist::Encode(playlist->dynamic_search()) : std::string());
    query.Bind(8, playlist->id());
    query.Exec();
    SqlQuery clear(database_, "DELETE FROM playlist_items WHERE playlist = ?");
    clear.Bind(1, playlist->id());
    clear.Exec();
  }
  for (int i = 0; i < playlist->row_count(); ++i) {
    SqlQuery item(database_, PlaylistItemBind::InsertSql());
    PlaylistItemBind::BindInsert(&item, playlist->id(), playlist->UuidAt(i), playlist->song(i));
    item.Exec();
  }
  return playlist->id();
}

void PlaylistBackend::SavePlaylistItems(int id, const std::vector<std::string> &uuids, const SongList &songs) {
  if (id < 0 || uuids.size() != songs.size()) {
    return;
  }
  for (size_t i = 0; i < songs.size(); ++i) {
    if (!PlaylistItemUuid::Valid(uuids[i])) {
      continue;
    }
    SqlQuery query(database_, PlaylistItemBind::UpdateSql());
    PlaylistItemBind::BindUpdate(&query, id, uuids[i], songs[i]);
    query.Exec();
  }
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
