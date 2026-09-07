//! Collection tree: group songs the way Strawberry's collection model does.

use orange_core::song::Song;

use crate::filter::CollectionFilter;

/// How the collection sidebar groups rows. Default matches 2.1.5:
/// Album artist → Album.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum GroupBy {
    #[default]
    AlbumArtistAlbum,
    ArtistAlbum,
    GenreArtistAlbum,
    Album,
    YearAlbum,
}

impl GroupBy {
    pub const ALL: &[GroupBy] = &[
        Self::AlbumArtistAlbum,
        Self::ArtistAlbum,
        Self::GenreArtistAlbum,
        Self::Album,
        Self::YearAlbum,
    ];

    pub fn label(self) -> &'static str {
        match self {
            Self::AlbumArtistAlbum => "Album artist / Album",
            Self::ArtistAlbum => "Artist / Album",
            Self::GenreArtistAlbum => "Genre / Artist / Album",
            Self::Album => "Album",
            Self::YearAlbum => "Year / Album",
        }
    }
}

/// One album in the tree, with its tracks already sorted.
#[derive(Debug, Clone, PartialEq)]
pub struct AlbumNode {
    pub name: String,
    pub year: i64,
    pub songs: Vec<Song>,
}

/// Top-level collection node (artist, genre, or year depending on grouping).
#[derive(Debug, Clone, PartialEq)]
pub struct ArtistNode {
    pub name: String,
    pub albums: Vec<AlbumNode>,
}

impl ArtistNode {
    pub fn song_count(&self) -> usize {
        self.albums.iter().map(|a| a.songs.len()).sum()
    }
}

/// Totals shown in the collection status line.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct CollectionStats {
    pub songs: usize,
    pub albums: usize,
    pub artists: usize,
}

/// Group `songs` into a tree, applying the free-text filter first.
pub fn build_tree(songs: &[Song], group_by: GroupBy, filter: &CollectionFilter) -> Vec<ArtistNode> {
    let filtered: Vec<&Song> = songs.iter().filter(|s| filter.matches(s)).collect();
    let mut artists: Vec<ArtistNode> = Vec::new();

    for song in filtered {
        let (top, album_name) = group_keys(song, group_by);
        let artist = if let Some(existing) = artists.iter_mut().find(|a| a.name == top) {
            existing
        } else {
            artists.push(ArtistNode {
                name: top.clone(),
                albums: Vec::new(),
            });
            artists.last_mut().unwrap()
        };
        let album = if let Some(existing) = artist.albums.iter_mut().find(|a| a.name == album_name)
        {
            existing
        } else {
            artist.albums.push(AlbumNode {
                name: album_name.clone(),
                year: if song.year > 0 { song.year } else { 0 },
                songs: Vec::new(),
            });
            artist.albums.last_mut().unwrap()
        };
        if song.year > album.year {
            album.year = song.year;
        }
        album.songs.push(song.clone());
    }

    for artist in &mut artists {
        for album in &mut artist.albums {
            album.songs.sort_by(|a, b| {
                a.disc
                    .cmp(&b.disc)
                    .then(a.track.cmp(&b.track))
                    .then(a.display_title().cmp(&b.display_title()))
            });
        }
        artist.albums.sort_by(|a, b| {
            a.year.cmp(&b.year).then(
                a.name
                    .to_ascii_lowercase()
                    .cmp(&b.name.to_ascii_lowercase()),
            )
        });
    }
    artists.sort_by(|a, b| {
        a.name
            .to_ascii_lowercase()
            .cmp(&b.name.to_ascii_lowercase())
    });
    artists
}

fn group_keys(song: &Song, group_by: GroupBy) -> (String, String) {
    match group_by {
        GroupBy::AlbumArtistAlbum => (
            song.effective_albumartist(),
            song.display_album().to_string(),
        ),
        GroupBy::ArtistAlbum => (
            song.display_artist().to_string(),
            song.display_album().to_string(),
        ),
        GroupBy::GenreArtistAlbum => {
            let genre = if song.genre.trim().is_empty() {
                "Unknown Genre".to_string()
            } else {
                song.genre.clone()
            };
            (
                genre,
                format!("{} — {}", song.display_artist(), song.display_album()),
            )
        }
        GroupBy::Album => (
            song.display_album().to_string(),
            song.display_album().to_string(),
        ),
        GroupBy::YearAlbum => {
            let year = if song.year > 0 {
                song.year.to_string()
            } else {
                "Unknown Year".to_string()
            };
            (year, song.display_album().to_string())
        }
    }
}

/// Flatten a tree back into playable songs (artist/album order).
pub fn flatten_songs(tree: &[ArtistNode]) -> Vec<Song> {
    let mut out = Vec::new();
    for artist in tree {
        for album in &artist.albums {
            out.extend(album.songs.iter().cloned());
        }
    }
    out
}

pub fn stats(tree: &[ArtistNode]) -> CollectionStats {
    CollectionStats {
        songs: tree.iter().map(ArtistNode::song_count).sum(),
        albums: tree.iter().map(|a| a.albums.len()).sum(),
        artists: tree.len(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use orange_core::song::Song;

    fn song(artist: &str, album: &str, title: &str, track: i64) -> Song {
        Song {
            artist: artist.into(),
            albumartist: artist.into(),
            album: album.into(),
            title: title.into(),
            track,
            year: 1959,
            url: format!("file:///m/{title}.flac"),
            ..Song::default()
        }
    }

    #[test]
    fn groups_album_artist_then_album() {
        let songs = vec![
            song("Miles Davis", "Kind of Blue", "So What", 1),
            song("Miles Davis", "Kind of Blue", "Freddie", 2),
            song("John Coltrane", "Giant Steps", "Giant Steps", 1),
        ];
        let tree = build_tree(
            &songs,
            GroupBy::AlbumArtistAlbum,
            &CollectionFilter::default(),
        );
        assert_eq!(tree.len(), 2);
        assert_eq!(tree[1].name, "Miles Davis");
        assert_eq!(tree[1].albums[0].songs.len(), 2);
        assert_eq!(tree[1].albums[0].songs[0].title, "So What");
        let totals = stats(&tree);
        assert_eq!(totals.songs, 3);
        assert_eq!(totals.albums, 2);
        assert_eq!(totals.artists, 2);
    }

    #[test]
    fn filter_hides_non_matching_artists() {
        let songs = vec![
            song("Miles Davis", "Kind of Blue", "So What", 1),
            song("John Coltrane", "Giant Steps", "Giant Steps", 1),
        ];
        let filter = CollectionFilter::parse("coltrane");
        let tree = build_tree(&songs, GroupBy::AlbumArtistAlbum, &filter);
        assert_eq!(tree.len(), 1);
        assert_eq!(tree[0].name, "John Coltrane");
    }
}
