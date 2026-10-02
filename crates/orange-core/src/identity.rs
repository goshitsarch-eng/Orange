//! Permanent catalog identity.
//!
//! The desktop/AppStream catalog identity is `com.goshapps.Orange`.
//! This never renames the application name, organization name, settings keys,
//! library/database, configuration, or cache locations, and existing
//! Strawberry data is left in place, never moved or deleted.
//!
//! Mirrors `src/main.cpp` (`setApplicationName("Orange")`,
//! `setOrganizationName("Orange")`) and `src/core/database.cpp`
//! (`kDatabaseFilename = "orange.db"`).

/// Reverse-DNS catalog identity. Unchanged from 2.1.5.
pub const APP_ID: &str = "com.goshapps.Orange";
/// Application name shown to users. Unchanged.
pub const APP_NAME: &str = "Orange";
/// Full display name.
pub const APP_DISPLAY_NAME: &str = "Orange Music Player";
/// Organization name (QSettings scope). Unchanged.
pub const ORG_NAME: &str = "Orange";
/// Collection database filename inside the app data dir. Unchanged.
pub const DB_FILENAME: &str = "orange.db";
/// Lower-cased QSettings scope used on disk (`~/.config/orange/orange.conf`).
pub const SETTINGS_ORG: &str = "orange";
/// Lower-cased QSettings application used on disk.
pub const SETTINGS_APP: &str = "orange";

/// Legacy Strawberry locations. Orange never writes here, never migrates
/// these files, and never deletes them. They exist so the guard below can
/// prove Orange paths cannot collide with them.
pub const STRAWBERRY_ORG: &str = "strawberry";
pub const STRAWBERRY_APP: &str = "strawberry";
pub const STRAWBERRY_DB_FILENAME: &str = "strawberry.db";

/// App data directory for Orange, e.g. `~/.local/share/orange/orange`.
/// Mirrors `StandardPaths::WritableLocation(AppDataLocation)`.
pub fn app_data_dir(base: impl AsRef<std::path::Path>) -> std::path::PathBuf {
    base.as_ref().join(SETTINGS_ORG).join(SETTINGS_APP)
}

pub fn collection_db_path(base: impl AsRef<std::path::Path>) -> std::path::PathBuf {
    app_data_dir(base).join(DB_FILENAME)
}

pub fn cache_dir(base: impl AsRef<std::path::Path>) -> std::path::PathBuf {
    base.as_ref().join(SETTINGS_ORG).join(SETTINGS_APP)
}

/// True when `path` points inside a legacy Strawberry data/config tree.
/// Orange 3 must never create, migrate, or delete such a path.
pub fn is_strawberry_legacy_path(path: &str) -> bool {
    let lower = path.to_lowercase();
    lower.contains("/strawberry/strawberry")
        || lower.ends_with("/strawberry")
        || lower.ends_with("strawberry.db")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn app_id_stable() {
        assert_eq!(APP_ID, "com.goshapps.Orange");
        assert_eq!(APP_NAME, "Orange");
        assert_eq!(ORG_NAME, "Orange");
        assert_eq!(DB_FILENAME, "orange.db");
    }

    #[test]
    fn orange_paths_do_not_collide_with_strawberry() {
        let db = collection_db_path("/home/user/.local/share");
        assert_eq!(
            db,
            std::path::Path::new("/home/user/.local/share")
                .join("orange")
                .join("orange")
                .join("orange.db")
        );
        assert!(!is_strawberry_legacy_path(&db.to_string_lossy()));
        assert_eq!(
            cache_dir("/home/user/.cache"),
            std::path::Path::new("/home/user/.cache")
                .join("orange")
                .join("orange")
        );
    }

    #[test]
    fn legacy_strawberry_paths_are_guarded() {
        assert!(is_strawberry_legacy_path(
            "/home/user/.local/share/strawberry/strawberry/strawberry.db"
        ));
        assert!(is_strawberry_legacy_path(
            "/home/user/.config/strawberry/strawberry.conf"
        ));
        assert!(!is_strawberry_legacy_path(
            "/home/user/.local/share/orange/orange/orange.db"
        ));
    }
}
