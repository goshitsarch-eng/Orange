//! Playlists: item model, undoable editing, repeat/shuffle sequencing,
//! and M3U/XSPF/PLS parsing. Ports `playlist`, `playlistundocommand*`,
//! `playlistsequence`, and `playlistparsers`.

pub mod model;
pub mod parsers;
pub mod undo;
