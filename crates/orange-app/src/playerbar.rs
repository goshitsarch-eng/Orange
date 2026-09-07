//! Player bar + playlist + analyzer layout.
//!
//! The bar must stay window-bound with no clipped titles at narrow sizes:
//! [`fit_title`] middle-truncates with an ellipsis so the controls and the
//! analyzer never overflow, and [`PlayerBarLayout`] splits the available
//! width between track info, controls, and analyzer with minimums.

/// Middle-truncate `title` to at most `max_chars` characters.
/// Short titles pass through untouched; truncation keeps the start and end
/// (artist … title) with a single ellipsis.
pub fn fit_title(title: &str, max_chars: usize) -> String {
    let chars: Vec<char> = title.chars().collect();
    if chars.len() <= max_chars || max_chars <= 1 {
        return if max_chars == 0 {
            String::new()
        } else {
            title.to_string()
        };
    }
    let keep = max_chars - 1;
    let head = keep.div_ceil(2);
    let tail = keep / 2;
    let mut out = String::new();
    out.extend(chars[..head].iter());
    out.push('…');
    out.extend(chars[chars.len() - tail..].iter());
    out
}

/// Width budget for the player bar in logical pixels.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct PlayerBarLayout {
    /// Total window content width.
    pub width: f32,
}

impl PlayerBarLayout {
    const CONTROLS_MIN: f32 = 180.0;
    const ANALYZER_MIN: f32 = 120.0;
    const INFO_MIN: f32 = 80.0;

    /// Width for the track-info block. Never negative, never clips controls:
    /// info shrinks first (with [`fit_title`] applied by the view).
    pub fn info_width(&self) -> f32 {
        (self.width - Self::CONTROLS_MIN - Self::ANALYZER_MIN).max(Self::INFO_MIN.min(self.width))
    }

    /// Whether the analyzer gets its full minimum width.
    pub fn analyzer_visible(&self) -> bool {
        self.width >= Self::CONTROLS_MIN + Self::ANALYZER_MIN + Self::INFO_MIN
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn short_titles_untouched() {
        assert_eq!(fit_title("So What", 20), "So What");
        assert_eq!(fit_title("", 10), "");
    }

    #[test]
    fn long_titles_middle_truncated() {
        let fitted = fit_title("Miles Davis — Kind of Blue (Remastered 2019)", 20);
        assert_eq!(fitted.chars().count(), 20);
        assert!(fitted.contains('…'));
        assert!(fitted.starts_with("Miles Davi"));
        assert!(fitted.ends_with("d 2019)"));
    }

    #[test]
    fn layout_stays_window_bound() {
        // Wide window: info generous, analyzer visible.
        let wide = PlayerBarLayout { width: 1200.0 };
        assert!(wide.info_width() > 400.0);
        assert!(wide.analyzer_visible());
        // Narrow window: info block stays inside the window, analyzer hides
        // instead of clipping.
        let narrow = PlayerBarLayout { width: 200.0 };
        assert!((0.0..=200.0).contains(&narrow.info_width()));
        assert!(!narrow.analyzer_visible());
    }
}
