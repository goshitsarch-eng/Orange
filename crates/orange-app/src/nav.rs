//! Rhythmbox-style sources: Music / Play Queue / Playlists / Radio / Files /
//! Devices / Settings. The main window draws these as a left source list.

/// Every top-level source, in nav-bar order.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Page {
    Library,
    Queue,
    Playlists,
    Radio,
    Files,
    Devices,
    Settings,
}

impl Page {
    pub const ALL: &[Page] = &[
        Self::Library,
        Self::Queue,
        Self::Playlists,
        Self::Radio,
        Self::Files,
        Self::Devices,
        Self::Settings,
    ];

    pub fn title(self) -> &'static str {
        match self {
            Self::Library => "Music",
            Self::Queue => "Play Queue",
            Self::Playlists => "Playlists",
            Self::Radio => "Radio",
            Self::Files => "Files",
            Self::Devices => "Devices",
            Self::Settings => "Settings",
        }
    }

    /// Icon key resolved through [`orange_theme::nav_icon_name`].
    pub fn icon_key(self) -> &'static str {
        match self {
            Self::Library => "collection",
            Self::Queue => "queue",
            Self::Playlists => "playlists",
            Self::Radio => "radio",
            Self::Files => "files",
            Self::Devices => "devices",
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
                "Music",
                "Play Queue",
                "Playlists",
                "Radio",
                "Files",
                "Devices",
                "Settings"
            ]
        );
        for page in Page::ALL {
            assert!(!orange_theme::nav_icon_name(page.icon_key()).is_empty());
        }
    }
}
