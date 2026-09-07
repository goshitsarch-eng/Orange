//! Navigation: Collection / Playlists / Now Playing / Lyrics / Devices /
//! Radio / Settings. One enum drives the libcosmic nav bar, the headless
//! shell, and MPRIS-adjacent routing.

/// Every top-level page, in nav-bar order.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Page {
    Collection,
    Playlists,
    NowPlaying,
    Lyrics,
    Devices,
    Radio,
    Settings,
}

impl Page {
    pub const ALL: &[Page] = &[
        Self::Collection,
        Self::Playlists,
        Self::NowPlaying,
        Self::Lyrics,
        Self::Devices,
        Self::Radio,
        Self::Settings,
    ];

    pub fn title(self) -> &'static str {
        match self {
            Self::Collection => "Collection",
            Self::Playlists => "Playlists",
            Self::NowPlaying => "Now Playing",
            Self::Lyrics => "Lyrics",
            Self::Devices => "Devices",
            Self::Radio => "Radio",
            Self::Settings => "Settings",
        }
    }

    /// Icon key resolved through [`orange_theme::nav_icon_name`].
    pub fn icon_key(self) -> &'static str {
        match self {
            Self::Collection => "collection",
            Self::Playlists => "playlists",
            Self::NowPlaying => "now-playing",
            Self::Lyrics => "lyrics",
            Self::Devices => "devices",
            Self::Radio => "radio",
            Self::Settings => "settings",
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn nav_covers_all_required_pages() {
        let titles: Vec<_> = Page::ALL.iter().map(|p| p.title()).collect();
        assert_eq!(
            titles,
            [
                "Collection",
                "Playlists",
                "Now Playing",
                "Lyrics",
                "Devices",
                "Radio",
                "Settings"
            ]
        );
        for page in Page::ALL {
            assert!(!orange_theme::nav_icon_name(page.icon_key()).is_empty());
        }
    }
}
