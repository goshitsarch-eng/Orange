//! Smart and dynamic playlists: search-term AST plus SQL generation.
//! Ports `smartplaylistsearch`, `searchterm`, and `querygenerator`.
//! Pure logic, no UI dependency: the cosmic wizard page renders these types.

pub mod generator;
pub mod search;
