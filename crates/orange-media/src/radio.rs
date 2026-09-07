//! Internet radio: Radio Paradise, SomaFM, Radio Browser, custom streams.
//! Mirrors `radios` (+ `radioparadise`, `somafm`, `radiobrowser`).

/// A playable radio stream.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RadioStream {
    pub name: String,
    pub url: String,
}

/// A radio service with its channel catalogue.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RadioService {
    pub id: &'static str,
    pub name: &'static str,
    pub homepage: &'static str,
    pub streams: Vec<RadioStream>,
}

/// Radio Paradise channels, mirroring the 2.1.5 service.
pub fn radio_paradise() -> RadioService {
    RadioService {
        id: "radioparadise",
        name: "Radio Paradise",
        homepage: "https://radioparadise.com",
        streams: ["Main", "Mellow", "Rock", "World/Etc"]
            .iter()
            .map(|channel| RadioStream {
                name: channel.to_string(),
                url: format!(
                    "https://stream.radioparadise.com/{}",
                    channel.to_ascii_lowercase().replace('/', "-")
                ),
            })
            .collect(),
    }
}

/// SomaFM channels (representative subset; full list refreshes from SomaFM).
pub fn somafm() -> RadioService {
    RadioService {
        id: "somafm",
        name: "SomaFM",
        homepage: "https://somafm.com",
        streams: [
            "Groove Salad",
            "Drone Zone",
            "Deep Space One",
            "Secret Agent",
            "Fluid",
        ]
        .iter()
        .map(|channel| {
            let slug: String = channel
                .trim()
                .to_ascii_lowercase()
                .chars()
                .filter(|c| *c != ' ')
                .collect();
            RadioStream {
                name: channel.trim().to_string(),
                url: format!("https://ice1.somafm.com/{slug}-128-mp3"),
            }
        })
        .collect(),
    }
}

/// Radio Browser API base, mirroring the 2.1.5 client.
pub const RADIO_BROWSER_API_BASE: &str = "https://de1.api.radio-browser.info";

/// A user-added custom stream. Validated before it is stored.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CustomStream {
    pub name: String,
    pub url: String,
}

/// Accept only playable remote URLs, mirroring the add-stream dialog.
pub fn validate_custom_stream(name: &str, url: &str) -> Option<CustomStream> {
    let name = name.trim();
    let url = url.trim();
    if name.is_empty() || url.is_empty() {
        return None;
    }
    let lower = url.to_ascii_lowercase();
    if lower.starts_with("http://") || lower.starts_with("https://") {
        Some(CustomStream {
            name: name.to_string(),
            url: url.to_string(),
        })
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn services_have_streams() {
        let rp = radio_paradise();
        assert_eq!(rp.streams.len(), 4);
        assert!(rp.streams.iter().all(|s| s.url.starts_with("https://")));
        let soma = somafm();
        assert!(soma.streams.iter().any(|s| s.name == "Groove Salad"));
        assert!(!RADIO_BROWSER_API_BASE.is_empty());
    }

    #[test]
    fn custom_stream_validation() {
        assert!(validate_custom_stream("X", "https://example.com/stream").is_some());
        assert!(validate_custom_stream("", "https://example.com/stream").is_none());
        assert!(validate_custom_stream("X", "file:///etc/passwd").is_none());
        assert!(validate_custom_stream("X", "javascript:alert(1)").is_none());
    }
}
