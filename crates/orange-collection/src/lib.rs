//! Collection: query builder, text filter, and directory-scan diff.
//! Ports `collectionquery`, `collectionfilter`, and the pure part of
//! `collectionwatcher` (diffing a scan against known files). The live
//! filesystem watch itself is a thin `notify` wrapper in the shell.

pub mod filter;
pub mod query;
pub mod watcher;
