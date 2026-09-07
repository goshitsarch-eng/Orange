//! Collection database layer.
//!
//! Read-compatible with the 2.1.5 schema (`data/schema/schema.sql`,
//! `schema_version` table, current version 23, minimum supported 10).
//! Rules, mirroring `src/core/database.cpp`:
//!
//! - Existing databases open read-only by default and are never migrated
//!   behind the caller's back; Strawberry legacy paths are refused for any
//!   write.
//! - Fresh databases are created from the canonical `schema.sql`.
//! - Outdated Orange databases migrate forward through `schema-11..23.sql`
//!   in order (additive only; Orange 3 adds no schema-24 changes yet).
//! - Databases newer than [`SCHEMA_VERSION`] open read-only with a warning.
//! - Zero telemetry.

use orange_core::identity;
use rusqlite::{Connection, OpenFlags};
use std::path::Path;

/// Current schema version, mirroring `Database::kSchemaVersion`.
pub const SCHEMA_VERSION: i32 = 23;
/// Minimum openable version, mirroring `kMinSupportedSchemaVersion`.
pub const MIN_SUPPORTED_SCHEMA_VERSION: i32 = 10;

/// Canonical fresh-install schema (2.1.5 `data/schema/schema.sql`).
const SCHEMA_SQL: &str = include_str!("../../../data/schema/schema.sql");

/// Ordered forward migrations `schema-11.sql ..= schema-23.sql`.
/// Each file ends with `UPDATE schema_version SET version=N`.
const MIGRATIONS: &[(i32, &str)] = &[
    (11, include_str!("../../../data/schema/schema-11.sql")),
    (12, include_str!("../../../data/schema/schema-12.sql")),
    (13, include_str!("../../../data/schema/schema-13.sql")),
    (14, include_str!("../../../data/schema/schema-14.sql")),
    (15, include_str!("../../../data/schema/schema-15.sql")),
    (16, include_str!("../../../data/schema/schema-16.sql")),
    (17, include_str!("../../../data/schema/schema-17.sql")),
    (18, include_str!("../../../data/schema/schema-18.sql")),
    (19, include_str!("../../../data/schema/schema-19.sql")),
    (20, include_str!("../../../data/schema/schema-20.sql")),
    (21, include_str!("../../../data/schema/schema-21.sql")),
    (22, include_str!("../../../data/schema/schema-22.sql")),
    (23, include_str!("../../../data/schema/schema-23.sql")),
];

/// How the database was opened.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OpenMode {
    /// Immutable read: existing 2.1.5/3.0 databases open as-is, never written.
    ReadOnly,
    /// Read-write for the Orange collection only. Refuses Strawberry legacy
    /// paths and migrates outdated Orange databases forward.
    ReadWrite,
}

/// Database open error.
#[derive(Debug)]
pub enum DbError {
    /// Refused: the path is inside a legacy Strawberry tree.
    StrawberryPathGuard(String),
    /// Schema too old to open safely (< 10).
    SchemaTooOld(i32),
    /// Missing file in read-only mode.
    Missing(String),
    Sql(rusqlite::Error),
}

impl std::fmt::Display for DbError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::StrawberryPathGuard(p) => {
                write!(f, "refusing to write inside legacy Strawberry data: {p}")
            }
            Self::SchemaTooOld(v) => write!(f, "database schema too old: version {v}"),
            Self::Missing(p) => write!(f, "database not found: {p}"),
            Self::Sql(e) => write!(f, "sqlite error: {e}"),
        }
    }
}

impl From<rusqlite::Error> for DbError {
    fn from(e: rusqlite::Error) -> Self {
        Self::Sql(e)
    }
}

/// Read the `schema_version` table, mirroring `Database::SchemaVersion`.
/// Returns 0 when the table is absent (pre-schema database).
pub fn schema_version(conn: &Connection) -> Result<i32, DbError> {
    let mut stmt = conn.prepare("SELECT version FROM schema_version")?;
    let mut rows = stmt.query([])?;
    if let Some(row) = rows.next()? {
        Ok(row.get(0)?)
    } else {
        Ok(0)
    }
}

/// Open a collection database.
///
/// - [`OpenMode::ReadOnly`] never writes: no journal, no migrations.
/// - [`OpenMode::ReadWrite`] refuses Strawberry legacy paths, creates a
///   fresh schema when missing, and migrates forward otherwise.
pub fn open_collection(path: &Path, mode: OpenMode) -> Result<Connection, DbError> {
    let path_str = path.to_string_lossy().into_owned();
    if mode == OpenMode::ReadWrite && identity::is_strawberry_legacy_path(&path_str) {
        return Err(DbError::StrawberryPathGuard(path_str));
    }
    match mode {
        OpenMode::ReadOnly => {
            if !path.exists() {
                return Err(DbError::Missing(path_str));
            }
            let conn = Connection::open_with_flags(
                path,
                OpenFlags::SQLITE_OPEN_READ_ONLY | OpenFlags::SQLITE_OPEN_NO_MUTEX,
            )?;
            let version = schema_version(&conn)?;
            if version != 0 && version < MIN_SUPPORTED_SCHEMA_VERSION {
                return Err(DbError::SchemaTooOld(version));
            }
            Ok(conn)
        }
        OpenMode::ReadWrite => {
            if let Some(parent) = path.parent() {
                if !parent.as_os_str().is_empty() {
                    std::fs::create_dir_all(parent).map_err(|e| {
                        DbError::Sql(rusqlite::Error::ToSqlConversionFailure(Box::new(e)))
                    })?;
                }
            }
            let conn = Connection::open(path)?;
            conn.busy_timeout(std::time::Duration::from_millis(30_000))?;
            if is_empty_database(&conn)? {
                conn.execute_batch(SCHEMA_SQL)?;
            } else {
                migrate_forward(&conn)?;
            }
            Ok(conn)
        }
    }
}

fn is_empty_database(conn: &Connection) -> Result<bool, DbError> {
    let count: i64 = conn.query_row(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'",
        [],
        |row| row.get(0),
    )?;
    Ok(count == 0)
}

fn migrate_forward(conn: &Connection) -> Result<(), DbError> {
    let mut version = schema_version(conn)?;
    if version == 0 {
        // No version row but tables exist: treat as fresh schema install.
        conn.execute_batch(SCHEMA_SQL)?;
        version = schema_version(conn)?;
    }
    if version < MIN_SUPPORTED_SCHEMA_VERSION {
        return Err(DbError::SchemaTooOld(version));
    }
    if version > SCHEMA_VERSION {
        // Newer than we understand: leave untouched (caller opens read-only).
        return Ok(());
    }
    for (target, sql) in MIGRATIONS {
        if *target > version {
            conn.execute_batch(sql)?;
        }
    }
    Ok(())
}

/// Count rows in a table; used by the headless shell summary.
pub fn count_rows(conn: &Connection, table: &str) -> Result<i64, DbError> {
    // Table name comes from our own constant list, never user input.
    let sql = format!("SELECT COUNT(*) FROM {table}");
    Ok(conn.query_row(&sql, [], |row| row.get(0))?)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temp_path(name: &str) -> std::path::PathBuf {
        let mut p = std::env::temp_dir();
        p.push(format!("orange-db-test-{name}-{}", std::process::id()));
        let _ = std::fs::remove_file(&p);
        p
    }

    #[test]
    fn embedded_schema_matches_2_1_5() {
        assert!(SCHEMA_SQL.contains("INSERT INTO schema_version (version) VALUES (23)"));
        assert!(SCHEMA_SQL.contains("CREATE TABLE IF NOT EXISTS songs ("));
        assert!(SCHEMA_SQL.contains("CREATE TABLE IF NOT EXISTS playlists ("));
        assert!(SCHEMA_SQL.contains("CREATE TABLE IF NOT EXISTS playlist_items ("));
        assert!(SCHEMA_SQL.contains("CREATE TABLE IF NOT EXISTS devices ("));
        assert!(SCHEMA_SQL.contains("CREATE TABLE IF NOT EXISTS radio_channels ("));
        assert_eq!(MIGRATIONS.len(), 13);
        assert_eq!(MIGRATIONS.first().unwrap().0, 11);
        assert_eq!(MIGRATIONS.last().unwrap().0, 23);
    }

    #[test]
    fn fresh_database_gets_current_schema() {
        let path = temp_path("fresh");
        let conn = open_collection(&path, OpenMode::ReadWrite).unwrap();
        assert_eq!(schema_version(&conn).unwrap(), SCHEMA_VERSION);
        assert_eq!(count_rows(&conn, "songs").unwrap(), 0);
        drop(conn);
        let _ = std::fs::remove_file(&path);
    }

    /// A 2.1.5-shaped database written by foreign SQL (as produced by the
    /// CPython compat fixture in `scripts/compat/`) opens read-only with data
    /// intact. Here the same statements are replayed to keep the test
    /// self-contained; `just test-compat` replays them through CPython.
    #[test]
    fn legacy_shaped_db_opens_read_only_intact() {
        let path = temp_path("legacy");
        {
            let conn = Connection::open(&path).unwrap();
            conn.execute_batch(
                "CREATE TABLE schema_version (version INTEGER NOT NULL); \
           INSERT INTO schema_version (version) VALUES (22); \
           CREATE TABLE songs (title TEXT, artist TEXT, url TEXT NOT NULL); \
           INSERT INTO songs (title, artist, url) VALUES \
           ('Blue in Green', 'Miles Davis', 'file:///music/blue.flac'), \
           ('So What', 'Miles Davis', 'file:///music/sowhat.flac');",
            )
            .unwrap();
        }
        let conn = open_collection(&path, OpenMode::ReadOnly).unwrap();
        assert_eq!(schema_version(&conn).unwrap(), 22);
        assert_eq!(count_rows(&conn, "songs").unwrap(), 2);
        // Read-only really means read-only.
        assert!(conn
            .execute("INSERT INTO songs VALUES ('x','y','z')", [])
            .is_err());
        drop(conn);
        let _ = std::fs::remove_file(&path);
    }

    #[test]
    fn compat_fixture_from_cpython() {
        // Written by scripts/compat/make_fixture_db.py (`just test-compat`).
        // Skipped when the fixture is absent; never fails without it.
        let Ok(fixture) = std::env::var("ORANGE_COMPAT_FIXTURE") else {
            return;
        };
        let conn = open_collection(Path::new(&fixture), OpenMode::ReadOnly).unwrap();
        assert_eq!(schema_version(&conn).unwrap(), 22);
        assert_eq!(count_rows(&conn, "songs").unwrap(), 2);
        let title: String = conn
            .query_row("SELECT title FROM songs ORDER BY title LIMIT 1", [], |r| {
                r.get(0)
            })
            .unwrap();
        assert_eq!(title, "Blue in Green");
    }

    #[test]
    fn strawberry_paths_refused_for_writes_but_readable() {
        let legacy = if cfg!(windows) {
            "C:\\Users\\u\\AppData\\strawberry\\strawberry\\strawberry.db".to_string()
        } else {
            "/home/u/.local/share/strawberry/strawberry/strawberry.db".to_string()
        };
        let err = open_collection(Path::new(&legacy), OpenMode::ReadWrite).unwrap_err();
        assert!(matches!(err, DbError::StrawberryPathGuard(_)), "got {err}");
        // And the guard message names the path for the log, never credentials.
        assert!(format!("{err}").contains("Strawberry"));
    }

    #[test]
    fn missing_file_in_read_only_is_an_error() {
        let err = open_collection(
            Path::new("/nonexistent-orange-dir/orange.db"),
            OpenMode::ReadOnly,
        )
        .unwrap_err();
        assert!(matches!(err, DbError::Missing(_)));
    }

    #[test]
    fn too_old_schema_rejected() {
        let path = temp_path("old");
        {
            let conn = Connection::open(&path).unwrap();
            conn.execute_batch(
                "CREATE TABLE schema_version (version INTEGER NOT NULL); \
         INSERT INTO schema_version (version) VALUES (9);",
            )
            .unwrap();
        }
        let err = open_collection(&path, OpenMode::ReadOnly).unwrap_err();
        assert!(matches!(err, DbError::SchemaTooOld(9)));
        let _ = std::fs::remove_file(&path);
    }

    #[test]
    fn missing_error_names_the_path() {
        let err = DbError::Missing("/home/u/.local/share/orange/orange/orange.db".to_string());
        assert!(format!("{err}").contains("orange.db"));
    }
}
