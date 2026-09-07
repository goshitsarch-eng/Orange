//! Appearance mode: System / Light / Dark (Settings > Appearance).
//! System follows the portal ColorScheme and cosmic-config, switching live
//! with no restart. Both palettes are Orange's own and identical everywhere.

/// Settings > Appearance choice. Default is System.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum AppearanceMode {
    /// Follow the desktop color scheme (portal + cosmic-config), live.
    #[default]
    System,
    Light,
    Dark,
}

impl AppearanceMode {
    /// Parse the stored settings string. Unknown values fall back to System
    /// so a corrupt config can never strand the UI without a theme.
    pub fn parse(value: &str) -> Self {
        match value.trim().to_ascii_lowercase().as_str() {
            "light" => Self::Light,
            "dark" => Self::Dark,
            _ => Self::System,
        }
    }

    pub fn as_str(self) -> &'static str {
        match self {
            Self::System => "system",
            Self::Light => "light",
            Self::Dark => "dark",
        }
    }

    /// Resolve to a concrete dark/light flag given the system scheme.
    pub fn is_dark(self, system_prefers_dark: bool) -> bool {
        match self {
            Self::Light => false,
            Self::Dark => true,
            Self::System => system_prefers_dark,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_round_trip() {
        assert_eq!(AppearanceMode::parse("light"), AppearanceMode::Light);
        assert_eq!(AppearanceMode::parse("DARK"), AppearanceMode::Dark);
        assert_eq!(AppearanceMode::parse("system"), AppearanceMode::System);
        assert_eq!(AppearanceMode::parse(""), AppearanceMode::System);
        assert_eq!(AppearanceMode::parse("midnight"), AppearanceMode::System);
        assert_eq!(AppearanceMode::default(), AppearanceMode::System);
    }

    #[test]
    fn resolution_follows_system_only_in_system_mode() {
        assert!(AppearanceMode::System.is_dark(true));
        assert!(!AppearanceMode::System.is_dark(false));
        assert!(!AppearanceMode::Light.is_dark(true));
        assert!(AppearanceMode::Dark.is_dark(false));
    }
}
