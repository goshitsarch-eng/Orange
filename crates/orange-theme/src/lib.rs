//! Theme manager: Orange's own light/dark palettes, System/Light/Dark mode,
//! and Breeze-first icon preference.
//!
//! - Settings > Appearance is System / Light / Dark. System follows the
//!   portal ColorScheme plus cosmic-config and switches live with no restart
//!   (the `ui` feature subscribes to both; this type layer resolves the
//!   effective scheme).
//! - Both palettes are Orange's own and identical everywhere the app runs.
//! - Icons prefer Breeze when available, falling back to COSMIC icons.

use orange_core::appearance::AppearanceMode;

/// An sRGB color.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Color {
    pub r: u8,
    pub g: u8,
    pub b: u8,
}

impl Color {
    pub const fn new(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b }
    }

    pub fn css_hex(self) -> String {
        format!("#{:02X}{:02X}{:02X}", self.r, self.g, self.b)
    }
}

/// Orange's own palette. The accent is identical in both schemes.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Palette {
    pub background: Color,
    pub surface: Color,
    pub text: Color,
    pub text_secondary: Color,
    pub accent: Color,
    pub accent_text: Color,
    pub player_bar: Color,
}

impl Palette {
    /// Orange accent, shared by both schemes.
    pub const ACCENT: Color = Color::new(0xE8, 0x6A, 0x1B);

    pub fn light() -> Self {
        Self {
            background: Color::new(0xFA, 0xFA, 0xF8),
            surface: Color::new(0xFF, 0xFF, 0xFF),
            text: Color::new(0x1A, 0x1A, 0x1A),
            text_secondary: Color::new(0x5A, 0x5A, 0x5A),
            accent: Self::ACCENT,
            accent_text: Color::new(0xFF, 0xFF, 0xFF),
            player_bar: Color::new(0xF0, 0xED, 0xE8),
        }
    }

    pub fn dark() -> Self {
        Self {
            background: Color::new(0x1E, 0x1E, 0x24),
            surface: Color::new(0x28, 0x28, 0x30),
            text: Color::new(0xF2, 0xF0, 0xEA),
            text_secondary: Color::new(0xA8, 0xA4, 0x9C),
            accent: Self::ACCENT,
            accent_text: Color::new(0xFF, 0xFF, 0xFF),
            player_bar: Color::new(0x24, 0x24, 0x2B),
        }
    }
}

/// Resolved theme: which scheme is active and with what palette.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ResolvedTheme {
    pub dark: bool,
    pub palette: Palette,
}

/// Resolve the effective theme. `system_prefers_dark` comes from the portal
/// ColorScheme / cosmic-config subscription; pure here so the rule is tested
/// without a session bus.
pub fn resolve(mode: AppearanceMode, system_prefers_dark: bool) -> ResolvedTheme {
    let dark = mode.is_dark(system_prefers_dark);
    ResolvedTheme {
        dark,
        palette: if dark {
            Palette::dark()
        } else {
            Palette::light()
        },
    }
}

/// Icon theme preference, mirroring the 2.1.5 Breeze preference:
/// Breeze first when installed, COSMIC icons as fallback.
/// `available` is the set of installed theme names (lowercased).
pub fn preferred_icon_theme(available: &[&str]) -> &'static str {
    let has = |name: &str| available.iter().any(|t| t.eq_ignore_ascii_case(name));
    if has("breeze") || has("breeze-dark") {
        "Breeze"
    } else {
        "COSMIC"
    }
}

/// Standard icon base directories, in lookup order: user, Flatpak exports,
/// system. Mirrors XDG icon theme discovery.
pub fn system_icon_bases() -> Vec<std::path::PathBuf> {
    let mut bases = Vec::new();
    if let Ok(home) = std::env::var("HOME") {
        bases.push(std::path::PathBuf::from(format!(
            "{home}/.local/share/icons"
        )));
    }
    bases.push(std::path::PathBuf::from("/app/share/icons"));
    bases.push(std::path::PathBuf::from("/usr/share/icons"));
    bases.push(std::path::PathBuf::from("/run/host/usr/share/icons"));
    bases
}

/// Find an installed Breeze icon directory, if any. The shell prefers it via
/// `cosmic::icon_theme::set_default("Breeze")`; otherwise COSMIC icons stay.
pub fn find_breeze_icon_dir(bases: &[std::path::PathBuf]) -> Option<std::path::PathBuf> {
    for base in bases {
        for variant in ["breeze", "breeze-dark"] {
            let candidate = base.join(variant);
            if candidate.is_dir() {
                return Some(candidate);
            }
        }
    }
    None
}

/// Symbolic icon name for a navigation page. Names exist in both Breeze and
/// COSMIC themes so the preference switch never breaks artwork.
pub fn nav_icon_name(page: &str) -> &'static str {
    match page {
        "collection" => "folder-music-symbolic",
        "playlists" => "view-media-playlist-symbolic",
        "now-playing" => "media-playback-start-symbolic",
        "lyrics" => "document-text-symbolic",
        "devices" => "drive-removable-media-symbolic",
        "radio" => "radio-symbolic",
        "settings" => "settings-symbolic",
        _ => "application-default-symbolic",
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn palettes_share_orange_accent() {
        assert_eq!(Palette::light().accent, Palette::dark().accent);
        assert_eq!(Palette::ACCENT.css_hex(), "#E86A1B");
    }

    #[test]
    fn system_mode_switches_live() {
        // No restart: same mode, new portal value, new theme.
        assert!(!resolve(AppearanceMode::System, false).dark);
        assert!(resolve(AppearanceMode::System, true).dark);
        assert!(!resolve(AppearanceMode::Light, true).dark);
        assert!(resolve(AppearanceMode::Dark, false).dark);
    }

    #[test]
    fn breeze_preferred_when_installed() {
        assert_eq!(preferred_icon_theme(&["breeze", "hicolor"]), "Breeze");
        assert_eq!(preferred_icon_theme(&["Breeze-Dark"]), "Breeze");
        assert_eq!(preferred_icon_theme(&["hicolor", "Adwaita"]), "COSMIC");
        assert_eq!(preferred_icon_theme(&[]), "COSMIC");
    }

    #[test]
    fn breeze_detection_uses_real_directories() {
        let dir = std::env::temp_dir().join(format!("orange-icons-{}", std::process::id()));
        let breeze = dir.join("breeze");
        std::fs::create_dir_all(&breeze).unwrap();
        assert_eq!(
            find_breeze_icon_dir(std::slice::from_ref(&dir)),
            Some(breeze)
        );
        assert_eq!(find_breeze_icon_dir(&[dir.join("elsewhere")]), None);
        let _ = std::fs::remove_dir_all(&dir);
        // This host ships Breeze (Plasma system), so the real lookup hits.
        if std::path::Path::new("/usr/share/icons/breeze").is_dir() {
            assert!(find_breeze_icon_dir(&system_icon_bases()).is_some());
        }
    }

    #[test]
    fn every_nav_page_has_an_icon() {
        for page in [
            "collection",
            "playlists",
            "now-playing",
            "lyrics",
            "devices",
            "radio",
            "settings",
        ] {
            assert!(!nav_icon_name(page).is_empty(), "{page}");
        }
    }
}
