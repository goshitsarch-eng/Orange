//! Collection directories, songs, and saved playlists.
//! Mirrors `CollectionBackend` add/remove/load against the 2.1.5 schema.

use orange_core::song::{Song, UNKNOWN};
use rusqlite::{params, Connection, OptionalExtension};

use crate::DbError;

/// One music folder watched by the collection.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MusicDirectory {
    pub id: i64,
    pub path: String,
    pub subdirs: bool,
}

/// A named playlist stored in `playlists`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SavedPlaylist {
    pub id: i64,
    pub name: String,
    pub favorite: bool,
}

/// List collection directories, oldest-id first (stable with add order).
pub fn list_directories(conn: &Connection) -> Result<Vec<MusicDirectory>, DbError> {
    let mut stmt = conn.prepare("SELECT ROWID, path, subdirs FROM directories ORDER BY path")?;
    let rows = stmt.query_map([], |row| {
        Ok(MusicDirectory {
            id: row.get(0)?,
            path: row.get(1)?,
            subdirs: row.get::<_, i64>(2)? != 0,
        })
    })?;
    let mut out = Vec::new();
    for row in rows {
        out.push(row?);
    }
    Ok(out)
}

/// Insert a music folder if it is not already present. Returns the row id.
pub fn add_directory(conn: &Connection, path: &str) -> Result<i64, DbError> {
    let path = path.trim_end_matches('/');
    if let Some(id) = conn
        .query_row(
            "SELECT ROWID FROM directories WHERE path = ?1",
            params![path],
            |row| row.get::<_, i64>(0),
        )
        .optional()?
    {
        return Ok(id);
    }
    conn.execute(
        "INSERT INTO directories (path, subdirs) VALUES (?1, 1)",
        params![path],
    )?;
    Ok(conn.last_insert_rowid())
}

/// Remove a directory and every song that belongs to it.
pub fn remove_directory(conn: &Connection, id: i64) -> Result<(), DbError> {
    conn.execute("DELETE FROM songs WHERE directory_id = ?1", params![id])?;
    conn.execute("DELETE FROM directories WHERE ROWID = ?1", params![id])?;
    Ok(())
}

/// Replace every song for `directory_id` with `songs` (one scan result).
pub fn replace_directory_songs(
    conn: &Connection,
    directory_id: i64,
    songs: &[Song],
) -> Result<usize, DbError> {
    let tx = conn.unchecked_transaction()?;
    tx.execute(
        "DELETE FROM songs WHERE directory_id = ?1",
        params![directory_id],
    )?;
    {
        let mut stmt = tx.prepare(
            "INSERT INTO songs (
                title, album, artist, albumartist, track, disc, year, genre,
                composer, performer, grouping, comment, lyrics, beginning, length,
                bitrate, samplerate, bitdepth, url, filesize, mtime, directory_id,
                unavailable, playcount, skipcount, lastplayed, rating,
                effective_albumartist, fingerprint
            ) VALUES (
                ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8,
                ?9, ?10, ?11, ?12, ?13, ?14, ?15,
                ?16, ?17, ?18, ?19, ?20, ?21, ?22,
                0, ?23, ?24, ?25, ?26,
                ?27, ?28
            )",
        )?;
        for song in songs {
            stmt.execute(params![
                song.title,
                song.album,
                song.artist,
                song.albumartist,
                song.track,
                song.disc,
                song.year,
                song.genre,
                song.composer,
                song.performer,
                song.grouping,
                song.comment,
                song.lyrics,
                song.beginning_ns,
                song.length_ns,
                song.bitrate,
                song.samplerate,
                song.bitdepth,
                song.url,
                song.filesize,
                song.mtime,
                directory_id,
                song.playcount,
                song.skipcount,
                song.lastplayed,
                song.rating as i64,
                song.effective_albumartist(),
                song.fingerprint,
            ])?;
        }
    }
    tx.commit()?;
    Ok(songs.len())
}

/// Load every available song, ordered like the 2.1.5 collection model.
pub fn load_songs(conn: &Connection) -> Result<Vec<Song>, DbError> {
    let mut stmt = conn.prepare(
        "SELECT title, album, artist, albumartist, track, disc, year, genre,
                composer, performer, grouping, comment, lyrics, beginning, length,
                bitrate, samplerate, bitdepth, url, filesize, mtime, directory_id,
                unavailable, playcount, skipcount, lastplayed, rating, fingerprint
         FROM songs
         WHERE unavailable IS NULL OR unavailable = 0
         ORDER BY albumartist COLLATE NOCASE, artist COLLATE NOCASE,
                  album COLLATE NOCASE, disc, track, title COLLATE NOCASE",
    )?;
    let rows = stmt.query_map([], song_from_row)?;
    let mut out = Vec::new();
    for row in rows {
        out.push(row?);
    }
    Ok(out)
}

fn song_from_row(row: &rusqlite::Row<'_>) -> rusqlite::Result<Song> {
    let rating_int: i64 = row.get(26)?;
    Ok(Song {
        title: row.get::<_, Option<String>>(0)?.unwrap_or_default(),
        album: row.get::<_, Option<String>>(1)?.unwrap_or_default(),
        artist: row.get::<_, Option<String>>(2)?.unwrap_or_default(),
        albumartist: row.get::<_, Option<String>>(3)?.unwrap_or_default(),
        track: row.get(4)?,
        disc: row.get(5)?,
        year: row.get(6)?,
        genre: row.get::<_, Option<String>>(7)?.unwrap_or_default(),
        composer: row.get::<_, Option<String>>(8)?.unwrap_or_default(),
        performer: row.get::<_, Option<String>>(9)?.unwrap_or_default(),
        grouping: row.get::<_, Option<String>>(10)?.unwrap_or_default(),
        comment: row.get::<_, Option<String>>(11)?.unwrap_or_default(),
        lyrics: row.get::<_, Option<String>>(12)?.unwrap_or_default(),
        beginning_ns: row.get(13)?,
        length_ns: row.get(14)?,
        bitrate: row.get(15)?,
        samplerate: row.get(16)?,
        bitdepth: row.get(17)?,
        url: row.get(18)?,
        filesize: row.get(19)?,
        mtime: row.get(20)?,
        directory_id: row.get(21)?,
        unavailable: row.get::<_, Option<i64>>(22)?.unwrap_or(0) != 0,
        playcount: row.get(23)?,
        skipcount: row.get(24)?,
        lastplayed: row.get(25)?,
        rating: rating_int as f64,
        fingerprint: row.get::<_, Option<String>>(27)?.unwrap_or_default(),
        ..Song::default()
    })
}

/// Known files for a directory, used by the collection watcher diff.
pub fn known_files(
    conn: &Connection,
    directory_id: i64,
) -> Result<Vec<orange_core::song::Song>, DbError> {
    let mut stmt =
        conn.prepare("SELECT url, mtime FROM songs WHERE directory_id = ?1 AND (unavailable IS NULL OR unavailable = 0)")?;
    let rows = stmt.query_map(params![directory_id], |row| {
        Ok(Song {
            url: row.get(0)?,
            mtime: row.get(1)?,
            ..Song::default()
        })
    })?;
    let mut out = Vec::new();
    for row in rows {
        out.push(row?);
    }
    Ok(out)
}

/// Saved playlists (name + id). Items are loaded separately.
pub fn list_playlists(conn: &Connection) -> Result<Vec<SavedPlaylist>, DbError> {
    let mut stmt =
        conn.prepare("SELECT ROWID, name, is_favorite FROM playlists ORDER BY ui_order, name")?;
    let rows = stmt.query_map([], |row| {
        Ok(SavedPlaylist {
            id: row.get(0)?,
            name: row.get(1)?,
            favorite: row.get::<_, i64>(2)? != 0,
        })
    })?;
    let mut out = Vec::new();
    for row in rows {
        out.push(row?);
    }
    Ok(out)
}

/// Load playlist item songs (URL + tags) for a saved playlist.
pub fn load_playlist_songs(conn: &Connection, playlist_id: i64) -> Result<Vec<Song>, DbError> {
    let mut stmt = conn.prepare(
        "SELECT title, album, artist, albumartist, track, year, genre, length, url
         FROM playlist_items WHERE playlist = ?1 ORDER BY ROWID",
    )?;
    let rows = stmt.query_map(params![playlist_id], |row| {
        Ok(Song {
            title: row.get::<_, Option<String>>(0)?.unwrap_or_default(),
            album: row.get::<_, Option<String>>(1)?.unwrap_or_default(),
            artist: row.get::<_, Option<String>>(2)?.unwrap_or_default(),
            albumartist: row.get::<_, Option<String>>(3)?.unwrap_or_default(),
            track: row.get::<_, Option<i64>>(4)?.unwrap_or(UNKNOWN),
            year: row.get::<_, Option<i64>>(5)?.unwrap_or(UNKNOWN),
            genre: row.get::<_, Option<String>>(6)?.unwrap_or_default(),
            length_ns: row.get::<_, Option<i64>>(7)?.unwrap_or(0),
            url: row.get::<_, Option<String>>(8)?.unwrap_or_default(),
            ..Song::default()
        })
    })?;
    let mut out = Vec::new();
    for row in rows {
        out.push(row?);
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{open_collection, OpenMode};
    use orange_core::song::Song;

    fn temp_path(name: &str) -> std::path::PathBuf {
        let mut p = std::env::temp_dir();
        p.push(format!(
            "orange-db-lib-{name}-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let _ = std::fs::remove_file(&p);
        p
    }

    fn sample(title: &str, artist: &str, album: &str) -> Song {
        Song {
            title: title.into(),
            artist: artist.into(),
            albumartist: artist.into(),
            album: album.into(),
            track: 1,
            year: 1959,
            url: format!("file:///music/{title}.flac"),
            length_ns: 185_000_000_000,
            ..Song::default()
        }
    }

    #[test]
    fn add_scan_load_remove_directory() {
        let path = temp_path("roundtrip");
        let conn = open_collection(&path, OpenMode::ReadWrite).unwrap();
        let id = add_directory(&conn, "/music/jazz").unwrap();
        assert_eq!(add_directory(&conn, "/music/jazz").unwrap(), id);
        let dirs = list_directories(&conn).unwrap();
        assert_eq!(dirs.len(), 1);
        assert_eq!(dirs[0].path, "/music/jazz");

        let songs = vec![
            sample("So What", "Miles Davis", "Kind of Blue"),
            sample("Blue in Green", "Miles Davis", "Kind of Blue"),
        ];
        assert_eq!(replace_directory_songs(&conn, id, &songs).unwrap(), 2);
        let loaded = load_songs(&conn).unwrap();
        assert_eq!(loaded.len(), 2);
        assert_eq!(loaded[0].artist, "Miles Davis");
        assert_eq!(loaded[0].directory_id, id);

        remove_directory(&conn, id).unwrap();
        assert!(list_directories(&conn).unwrap().is_empty());
        assert!(load_songs(&conn).unwrap().is_empty());
        drop(conn);
        let _ = std::fs::remove_file(&path);
    }

    #[test]
    fn saved_playlists_round_trip() {
        let path = temp_path("playlists");
        let conn = open_collection(&path, OpenMode::ReadWrite).unwrap();
        conn.execute(
            "INSERT INTO playlists (name, last_played, ui_order, is_favorite) VALUES ('Late night', -1, 0, 1)",
            [],
        )
        .unwrap();
        let id = conn.last_insert_rowid();
        conn.execute(
            "INSERT INTO playlist_items (playlist, type, title, artist, album, url, length)
             VALUES (?1, 0, 'So What', 'Miles Davis', 'Kind of Blue', 'file:///m/a.flac', 1000)",
            params![id],
        )
        .unwrap();
        let lists = list_playlists(&conn).unwrap();
        assert_eq!(lists[0].name, "Late night");
        assert!(lists[0].favorite);
        let items = load_playlist_songs(&conn, lists[0].id).unwrap();
        assert_eq!(items[0].title, "So What");
        drop(conn);
        let _ = std::fs::remove_file(&path);
    }
}
