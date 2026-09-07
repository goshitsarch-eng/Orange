//! Free-text collection filter (`artist:foo year:1959`).
//! Mirrors the `collectionfilter` field prefixes.

/// Parsed free-text filter: plain words plus `field:value` terms.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct CollectionFilter {
    /// Bare words, matched against title/artist/album.
    pub words: Vec<String>,
    /// Field-scoped terms in source order.
    pub terms: Vec<(String, String)>,
}

/// Fields the filter understands. Anything else stays a plain word so a
/// query like `remaster:2020` (a song title fragment) still matches.
const KNOWN_FIELDS: &[&str] = &[
    "artist",
    "album",
    "albumartist",
    "genre",
    "composer",
    "year",
];

impl CollectionFilter {
    pub fn parse(input: &str) -> Self {
        let mut filter = Self::default();
        for token in input.split_whitespace() {
            if let Some((field, value)) = token.split_once(':') {
                let field = field.to_ascii_lowercase();
                if KNOWN_FIELDS.contains(&field.as_str()) && !value.is_empty() {
                    filter.terms.push((field, value.to_string()));
                    continue;
                }
            }
            filter.words.push(token.to_string());
        }
        filter
    }

    /// Render as a `WHERE` fragment with bound params (see `query` module).
    pub fn where_clause(&self) -> (String, Vec<String>) {
        let mut sql = String::from("1=1");
        let mut params = Vec::new();
        for word in &self.words {
            sql.push_str(" AND (title LIKE ? OR artist LIKE ? OR album LIKE ?)");
            let like = format!("%{word}%");
            params.push(like.clone());
            params.push(like.clone());
            params.push(like);
        }
        for (field, value) in &self.terms {
            sql.push_str(" AND ");
            sql.push_str(field);
            sql.push_str(if field == "year" { " = ?" } else { " LIKE ?" });
            params.push(if field == "year" {
                value.clone()
            } else {
                format!("%{value}%")
            });
        }
        (sql, params)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_words_and_terms() {
        let f = CollectionFilter::parse("miles artist:davis 1959");
        assert_eq!(f.words, vec!["miles".to_string(), "1959".to_string()]);
        assert_eq!(f.terms, vec![("artist".to_string(), "davis".to_string())]);
    }

    #[test]
    fn unknown_field_stays_a_word() {
        let f = CollectionFilter::parse("remaster:2020");
        assert!(f.terms.is_empty());
        assert_eq!(f.words, vec!["remaster:2020".to_string()]);
    }

    #[test]
    fn where_clause_binds_everything() {
        let f = CollectionFilter::parse("blue artist:miles");
        let (sql, params) = f.where_clause();
        assert!(sql.contains("title LIKE ?"));
        assert!(sql.contains("AND artist LIKE ?"));
        assert_eq!(params.len(), 4);
    }
}
