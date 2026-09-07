//! Version and maker branding. Single source of truth for 3.0.0.

/// Orange 3 version. Must match Cargo workspace version, `--version`,
/// About, AppStream releases, README, and Changelog.
pub const VERSION: &str = "3.0.0";
/// Maker string shown in About, AppStream developer, and footer.
pub const MAKER: &str = "Gosh";
/// Upstream credit kept from 2.1.5: Orange is a fork of Strawberry
/// (itself forked from Clementine in 2018).
pub const UPSTREAM_CREDIT: &str =
    "Based on Strawberry (strawberrymusicplayer/strawberry), itself forked from Clementine (2018).";

/// `orange 3.0.0` line printed by `--version`.
pub fn version_line() -> String {
    format!("orange {}", VERSION)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_is_3_0_0() {
        assert_eq!(VERSION, "3.0.0");
        assert_eq!(version_line(), "orange 3.0.0");
        // Workspace Cargo.toml must agree; checked by orange-app's version test.
        assert_eq!(env!("CARGO_PKG_VERSION"), VERSION);
    }

    #[test]
    fn maker_is_gosh() {
        assert_eq!(MAKER, "Gosh");
    }
}
