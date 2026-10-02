//! Playlist file parsing: M3U (+EXTM3U), XSPF, and PLS.
//! Mirrors `playlistparsers/{m3u,xspf,pls}parser`.

use crate::model::PlaylistItem;

/// One parsed entry: URL plus optional metadata.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParsedEntry {
    pub url: String,
    pub title: String,
    pub length_secs: Option<i64>,
}

/// Parse an M3U/EXTM3U document. Relative paths stay relative, exactly as
/// the 2.1.5 parser leaves them for the caller to resolve.
pub fn parse_m3u(text: &str) -> Vec<ParsedEntry> {
    let mut entries = Vec::new();
    let mut pending_title = String::new();
    let mut pending_length: Option<i64> = None;
    for line in text.lines() {
        let line = line.trim().trim_start_matches('\u{feff}');
        if line.is_empty() {
            continue;
        }
        if let Some(rest) = line.strip_prefix("#EXTINF:") {
            let (length, title) = rest.split_once(',').unwrap_or((rest, ""));
            pending_length = length.trim().parse().ok().filter(|&n| n >= 0);
            pending_title = title.trim().to_string();
        } else if line.starts_with('#') {
            continue;
        } else {
            entries.push(ParsedEntry {
                url: line.to_string(),
                title: std::mem::take(&mut pending_title),
                length_secs: pending_length.take(),
            });
        }
    }
    entries
}

/// Serialize entries back to EXTM3U.
pub fn write_m3u(entries: &[ParsedEntry]) -> String {
    let mut out = String::from("#EXTM3U\n");
    for entry in entries {
        out.push_str(&format!(
            "#EXTINF:{},{}\n{}\n",
            entry.length_secs.unwrap_or(-1),
            entry.title.replace(['\r', '\n'], " "),
            entry.url.replace(['\r', '\n'], "")
        ));
    }
    out
}

/// Parse XML independently of whitespace and decode entities. Invalid XML
/// produces no tracks for legacy callers; interactive imports use the checked API.
pub fn parse_xspf(text: &str) -> Vec<ParsedEntry> {
    parse_xspf_checked(text).unwrap_or_default()
}

pub fn parse_xspf_checked(text: &str) -> Result<Vec<ParsedEntry>, String> {
    let doc = roxmltree::Document::parse_with_options(
        text,
        roxmltree::ParsingOptions {
            allow_dtd: false,
            nodes_limit: 100_000,
            ..Default::default()
        },
    )
    .map_err(|error| format!("Invalid XSPF: {error}"))?;
    if doc.root_element().tag_name().name() != "playlist" {
        return Err("XSPF root must be a playlist".into());
    }
    Ok(doc
        .descendants()
        .filter(|node| node.is_element() && node.tag_name().name() == "track")
        .filter_map(|node| {
            let value = |name| {
                node.children()
                    .find(|child| child.is_element() && child.tag_name().name() == name)
                    .and_then(|child| child.text())
            };
            let url = value("location")?.trim().to_owned();
            if url.is_empty() {
                return None;
            }
            Some(ParsedEntry {
                url,
                title: value("title").unwrap_or("").to_owned(),
                length_secs: value("duration")
                    .and_then(|v| v.parse::<i64>().ok())
                    .filter(|v| *v >= 0)
                    .map(|v| v / 1000),
            })
        })
        .collect())
}

/// Minimal PLS parse: `FileN=` + optional `TitleN=` pairs.
pub fn parse_pls(text: &str) -> Vec<ParsedEntry> {
    let mut files: Vec<(usize, String)> = Vec::new();
    let mut titles: Vec<(usize, String)> = Vec::new();
    for line in text.lines() {
        let line = line.trim();
        if let Some((key, value)) = line.split_once('=') {
            let key = key.trim();
            if let Some(n) = key
                .strip_prefix("File")
                .and_then(|n| n.parse::<usize>().ok())
            {
                files.push((n, value.trim().to_string()));
            } else if let Some(n) = key
                .strip_prefix("Title")
                .and_then(|n| n.parse::<usize>().ok())
            {
                titles.push((n, value.trim().to_string()));
            }
        }
    }
    files.sort_by_key(|(n, _)| *n);
    titles.sort_by_key(|(n, _)| *n);
    files
        .into_iter()
        .map(|(n, url)| {
            let title = titles
                .iter()
                .find(|(m, _)| *m == n)
                .map(|(_, t)| t.clone())
                .unwrap_or_default();
            ParsedEntry {
                url,
                title,
                length_secs: None,
            }
        })
        .collect()
}

impl From<ParsedEntry> for PlaylistItem {
    fn from(entry: ParsedEntry) -> Self {
        PlaylistItem::Stream {
            url: entry.url,
            title: entry.title,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn xspf_handles_compact_namespaced_xml_entities_and_crlf() {
        let entries = parse_xspf_checked("<playlist xmlns='http://xspf.org/ns/0/'><trackList>\r\n<track><location>https://example.org/a?x=1&amp;y=2</location><title>A &amp; B</title><duration>190000</duration></track></trackList></playlist>").unwrap();
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].url, "https://example.org/a?x=1&y=2");
        assert_eq!(entries[0].title, "A & B");
        assert_eq!(entries[0].length_secs, Some(190));
        assert!(parse_xspf_checked("<playlist><track>").is_err());
        assert!(parse_xspf_checked(
            "<!DOCTYPE playlist [<!ENTITY x SYSTEM 'file:///etc/passwd'>]><playlist>&x;</playlist>"
        )
        .is_err());
    }

    #[test]
    fn m3u_round_trip() {
        let text = "#EXTM3U\n#EXTINF:185,Blue in Green\nfile:///music/blue.flac\n# a comment\nhttp://stream/x\n";
        let entries = parse_m3u(text);
        assert_eq!(entries.len(), 2);
        assert_eq!(entries[0].title, "Blue in Green");
        assert_eq!(entries[0].length_secs, Some(185));
        assert_eq!(entries[1].url, "http://stream/x");
        let back = write_m3u(&entries);
        assert_eq!(parse_m3u(&back), entries);
    }

    #[test]
    fn xspf_locations_and_titles() {
        let text = "<playlist>\n<track>\n<location>file:///a.flac</location>\n<title>A</title>\n</track>\n<track>\n<location>file:///b.flac</location>\n</track>\n</playlist>";
        let entries = parse_xspf(text);
        assert_eq!(entries.len(), 2);
        assert_eq!(entries[0].title, "A");
        assert_eq!(entries[1].title, "");
    }

    #[test]
    fn pls_files_and_titles() {
        let text = "[playlist]\nFile1=http://a/x\nTitle1=A\nFile2=http://b/y\nNumberOfEntries=2\n";
        let entries = parse_pls(text);
        assert_eq!(entries.len(), 2);
        assert_eq!(entries[0].title, "A");
        assert_eq!(entries[1].title, "");
    }
}
