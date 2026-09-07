//! About dialog content: 3.0.0, Made by Gosh, upstream credits.

use orange_core::version::{MAKER, UPSTREAM_CREDIT, VERSION};

/// About dialog title.
pub fn title() -> String {
    format!("Orange {}", VERSION)
}

/// Maker line shown under the title.
pub fn maker_line() -> String {
    format!("Made by {MAKER}")
}

/// Full About body: version + maker + fork note + upstream credits.
pub fn body() -> String {
    format!(
        "Orange Music Player {VERSION}\n{maker}\n\n\
     A music player and collection organizer for audiophiles and collectors.\n\
     Orange is a fork of Strawberry.\n{UPSTREAM_CREDIT}\n\nMade by {MAKER}.",
        maker = maker_line(),
    )
}

/// Status-bar / footer credit.
pub fn footer() -> &'static str {
    "Made by Gosh"
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn about_shows_version_and_maker() {
        assert_eq!(title(), "Orange 3.0.0");
        assert_eq!(maker_line(), "Made by Gosh");
        assert_eq!(footer(), "Made by Gosh");
        let body = body();
        assert!(body.contains("3.0.0"));
        assert!(body.contains("Made by Gosh"));
        assert!(body.contains("Strawberry"));
        assert!(body.contains("Clementine"));
    }
}
