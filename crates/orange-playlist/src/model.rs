//! Playlist item model and sequencing.
//! Mirrors `PlaylistItem` (song vs. radio stream rows) and `PlaylistSequence`
//! (repeat/shuffle modes driving next/previous).

use orange_core::song::Song;

/// One playlist row: a library song or a stream URL.
#[derive(Debug, Clone, PartialEq)]
pub enum PlaylistItem {
    Song(Box<Song>),
    Stream { url: String, title: String },
}

impl PlaylistItem {
    pub fn title(&self) -> &str {
        match self {
            Self::Song(song) => {
                if song.title.is_empty() {
                    &song.url
                } else {
                    &song.title
                }
            }
            Self::Stream { title, url } => {
                if title.is_empty() {
                    url
                } else {
                    title
                }
            }
        }
    }
}

/// Repeat behavior, mirroring `PlaylistSequence::RepeatMode`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum RepeatMode {
    #[default]
    Off,
    Track,
    Album,
    Playlist,
}

/// Shuffle behavior, mirroring `PlaylistSequence::ShuffleMode`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum ShuffleMode {
    #[default]
    Off,
    All,
    InsideAlbum,
}

/// An ordered, editable list of items with a cursor.
#[derive(Debug, Clone, Default)]
pub struct Playlist {
    items: Vec<PlaylistItem>,
    cursor: Option<usize>,
}

impl Playlist {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn len(&self) -> usize {
        self.items.len()
    }

    pub fn is_empty(&self) -> bool {
        self.items.is_empty()
    }

    pub fn items(&self) -> &[PlaylistItem] {
        &self.items
    }

    pub fn cursor(&self) -> Option<usize> {
        self.cursor
    }

    pub fn current(&self) -> Option<&PlaylistItem> {
        self.cursor.and_then(|i| self.items.get(i))
    }

    pub fn insert(&mut self, index: usize, item: PlaylistItem) {
        let index = index.min(self.items.len());
        self.items.insert(index, item);
        if let Some(cursor) = self.cursor {
            if cursor >= index {
                self.cursor = Some(cursor + 1);
            }
        }
    }

    pub fn remove(&mut self, index: usize) -> Option<PlaylistItem> {
        if index >= self.items.len() {
            return None;
        }
        let removed = self.items.remove(index);
        self.cursor = match self.cursor {
            Some(cursor) if cursor == index => None,
            Some(cursor) if cursor > index => Some(cursor - 1),
            other => other,
        };
        Some(removed)
    }

    /// Move a row, adjusting the cursor like the 2.1.5 move command.
    pub fn move_row(&mut self, from: usize, to: usize) {
        if from >= self.items.len() || to >= self.items.len() || from == to {
            return;
        }
        let item = self.items.remove(from);
        self.items.insert(to, item);
        self.cursor = self.cursor.map(|cursor| {
            if cursor == from {
                to
            } else if from < cursor && cursor <= to {
                cursor - 1
            } else if to <= cursor && cursor < from {
                cursor + 1
            } else {
                cursor
            }
        });
    }

    /// Index of the next row under the given sequence modes.
    /// `order` is the shuffled play order when shuffling; otherwise ignored.
    pub fn next_index(
        &self,
        repeat: RepeatMode,
        shuffle: ShuffleMode,
        order: &[usize],
    ) -> Option<usize> {
        let cursor = self.cursor?;
        if repeat == RepeatMode::Track {
            return Some(cursor);
        }
        if shuffle != ShuffleMode::Off && !order.is_empty() {
            let pos = order.iter().position(|&i| i == cursor)?;
            if pos + 1 < order.len() {
                return Some(order[pos + 1]);
            }
            return if repeat == RepeatMode::Playlist {
                Some(order[0])
            } else {
                None
            };
        }
        if cursor + 1 < self.items.len() {
            Some(cursor + 1)
        } else if repeat == RepeatMode::Playlist {
            Some(0)
        } else {
            None
        }
    }
}

/// Deterministic shuffle order (Fisher-Yates over a seeded LCG) so tests
/// and "shuffle" previews are reproducible without an RNG dependency.
pub fn shuffled_order(len: usize, seed: u64) -> Vec<usize> {
    let mut order: Vec<usize> = (0..len).collect();
    let mut state = seed.wrapping_add(0x9E37_79B9_7F4A_7C15);
    for i in (1..len).rev() {
        state = state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        let j = (state >> 33) as usize % (i + 1);
        order.swap(i, j);
    }
    order
}

#[cfg(test)]
mod tests {
    use super::*;

    fn song(title: &str) -> PlaylistItem {
        PlaylistItem::Song(Box::new(orange_core::song::Song {
            title: title.to_string(),
            url: format!("file:///music/{title}.flac"),
            ..Default::default()
        }))
    }

    #[test]
    fn insert_adjusts_cursor() {
        let mut list = Playlist::new();
        list.insert(0, song("a"));
        list.insert(1, song("b"));
        list.cursor = Some(1);
        list.insert(0, song("z"));
        assert_eq!(list.cursor, Some(2));
        assert_eq!(list.current().unwrap().title(), "b");
    }

    #[test]
    fn remove_clears_cursor_on_current() {
        let mut list = Playlist::new();
        list.insert(0, song("a"));
        list.insert(1, song("b"));
        list.cursor = Some(0);
        assert_eq!(list.remove(0).unwrap().title(), "a");
        assert_eq!(list.cursor, None);
        assert!(list.remove(9).is_none());
    }

    #[test]
    fn move_row_tracks_cursor() {
        let mut list = Playlist::new();
        for title in ["a", "b", "c"] {
            list.insert(list.len(), song(title));
        }
        list.cursor = Some(0);
        list.move_row(0, 2);
        assert_eq!(list.cursor, Some(2));
        assert_eq!(list.items()[0].title(), "b");
    }

    #[test]
    fn sequencing_repeat_and_stop() {
        let mut list = Playlist::new();
        list.insert(0, song("a"));
        list.insert(1, song("b"));
        list.cursor = Some(1);
        assert_eq!(
            list.next_index(RepeatMode::Off, ShuffleMode::Off, &[]),
            None
        );
        assert_eq!(
            list.next_index(RepeatMode::Playlist, ShuffleMode::Off, &[]),
            Some(0)
        );
        assert_eq!(
            list.next_index(RepeatMode::Track, ShuffleMode::Off, &[]),
            Some(1)
        );
    }

    #[test]
    fn shuffle_is_deterministic_and_complete() {
        let first = shuffled_order(25, 7);
        assert_eq!(first, shuffled_order(25, 7));
        let mut sorted = first.clone();
        sorted.sort_unstable();
        assert_eq!(sorted, (0..25).collect::<Vec<_>>());
    }

    #[test]
    fn stream_title_falls_back_to_url() {
        let item = PlaylistItem::Stream {
            url: "http://x/y".into(),
            title: String::new(),
        };
        assert_eq!(item.title(), "http://x/y");
    }
}
