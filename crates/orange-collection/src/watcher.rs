//! Directory-scan diff: the pure core of the collection watcher.
//! Given the files seen on disk and the files known from the database,
//! report what was added, removed, or changed (by mtime). The live
//! filesystem subscription stays in the shell.

use std::collections::{HashMap, HashSet};

/// A file observed during a directory scan.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ScannedFile {
    pub url: String,
    pub mtime: i64,
    pub size: i64,
}

/// A file remembered from the database.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct KnownFile {
    pub url: String,
    pub mtime: i64,
}

/// Scan result split by action.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct ScanDiff {
    pub added: Vec<String>,
    pub removed: Vec<String>,
    pub changed: Vec<String>,
    pub unchanged: usize,
}

/// Diff a fresh scan against the known library state.
pub fn diff_scan(scanned: &[ScannedFile], known: &[KnownFile]) -> ScanDiff {
    let known_by_url: HashMap<&str, i64> =
        known.iter().map(|k| (k.url.as_str(), k.mtime)).collect();
    let scanned_urls: HashSet<&str> = scanned.iter().map(|s| s.url.as_str()).collect();
    let mut diff = ScanDiff::default();
    for file in scanned {
        match known_by_url.get(file.url.as_str()) {
            None => diff.added.push(file.url.clone()),
            Some(&mtime) if mtime != file.mtime => diff.changed.push(file.url.clone()),
            Some(_) => diff.unchanged += 1,
        }
    }
    for file in known {
        if !scanned_urls.contains(file.url.as_str()) {
            diff.removed.push(file.url.clone());
        }
    }
    diff
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn diff_covers_all_cases() {
        let known = vec![
            KnownFile {
                url: "a".into(),
                mtime: 1,
            },
            KnownFile {
                url: "b".into(),
                mtime: 2,
            },
            KnownFile {
                url: "gone".into(),
                mtime: 1,
            },
        ];
        let scanned = vec![
            ScannedFile {
                url: "a".into(),
                mtime: 1,
                size: 9,
            },
            ScannedFile {
                url: "b".into(),
                mtime: 3,
                size: 9,
            },
            ScannedFile {
                url: "c".into(),
                mtime: 1,
                size: 9,
            },
        ];
        let diff = diff_scan(&scanned, &known);
        assert_eq!(diff.added, vec!["c".to_string()]);
        assert_eq!(diff.changed, vec!["b".to_string()]);
        assert_eq!(diff.removed, vec!["gone".to_string()]);
        assert_eq!(diff.unchanged, 1);
    }

    #[test]
    fn empty_scan_removes_everything() {
        let known = vec![KnownFile {
            url: "a".into(),
            mtime: 1,
        }];
        let diff = diff_scan(&[], &known);
        assert_eq!(diff.removed, vec!["a".to_string()]);
        assert!(diff.added.is_empty());
    }
}
