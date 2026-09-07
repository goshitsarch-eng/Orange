//! Navigation: Collection / Playlists / Files / Radio / Devices / Settings.
//! One enum drives the libcosmic nav bar (the Strawberry-style source
//! sidebar). The playlist table stays visible for every source except
//! Settings.

/// Every top-level source, in nav-bar order (Strawberry's left tabs).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Page {
    Collection,
    Playlists,
    Files,
    Radio,
    Devices,
    Settings,
}

impl Page {
    pub const ALL: &[Page] = &[
        Self::Collection,
        Self::Playlists,
        Self::Files,
        Self::Radio,
        Self::Devices,
        Self::Settings,
    ];

    pub fn title(self) -> &'static str {
        match self {
            Self::Collection => "Collection",
            Self::Playlists => "Playlists",
            Self::Files => "Files",
            Self::Radio => "Radio",
            Self::Devices => "Devices",
            Self::Settings => "Settings",
        }
    }

    /// Icon key resolved through [`orange_theme::nav_icon_name`].
    pub fn icon_key(self) -> &'static str {
        match self {
            Self::Collection => "collection",
            Self::Playlists => "playlists",
            Self::Files => "files",
            Self::Radio => "radio",
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
                "Collection",
                "Playlists",
                "Files",
                "Radio",
                "Devices",
                "Settings"
            ]
        );
        for page in Page::ALL {
            assert!(!orange_theme::nav_icon_name(page.icon_key()).is_empty());
        }
    }
}
