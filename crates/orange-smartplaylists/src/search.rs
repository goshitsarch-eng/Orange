//! Search terms: field/operator/value triples combined into SQL.
//! Mirrors `SmartPlaylistSearchTerm` fields and operators.

/// A searchable song field. Names are the exact `songs` column names.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Field {
    Title,
    Artist,
    Album,
    AlbumArtist,
    Genre,
    Composer,
    Year,
    Track,
    Rating,
    PlayCount,
    SkipCount,
    LastPlayed,
    Length,
    Bitrate,
}

impl Field {
    pub fn column(self) -> &'static str {
        match self {
            Self::Title => "title",
            Self::Artist => "artist",
            Self::Album => "album",
            Self::AlbumArtist => "albumartist",
            Self::Genre => "genre",
            Self::Composer => "composer",
            Self::Year => "year",
            Self::Track => "track",
            Self::Rating => "rating",
            Self::PlayCount => "playcount",
            Self::SkipCount => "skipcount",
            Self::LastPlayed => "lastplayed",
            Self::Length => "length",
            Self::Bitrate => "bitrate",
        }
    }

    pub fn is_text(self) -> bool {
        matches!(
            self,
            Self::Title
                | Self::Artist
                | Self::Album
                | Self::AlbumArtist
                | Self::Genre
                | Self::Composer
        )
    }
}

/// Comparison operator, mirroring `SearchTerm::Operator`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Operator {
    Equals,
    NotEquals,
    Contains,
    NotContains,
    StartsWith,
    EndsWith,
    GreaterThan,
    LessThan,
}

/// One search term with its value kept as text (parsed per field kind).
#[derive(Debug, Clone, PartialEq)]
pub struct SearchTerm {
    pub field: Field,
    pub operator: Operator,
    pub value: String,
}

impl SearchTerm {
    /// Render as SQL with a `?` placeholder plus the bound value.
    /// Text matching uses LIKE with proper ESCAPE so `%` in values is literal.
    pub fn to_sql(&self) -> (String, String) {
        let column = self.field.column();
        if self.field.is_text() {
            let pattern = match self.operator {
                Operator::Equals => return (format!("{column} = ?"), self.value.clone()),
                Operator::NotEquals => return (format!("{column} != ?"), self.value.clone()),
                Operator::Contains | Operator::NotContains => {
                    format!("%{}%", escape_like(&self.value))
                }
                Operator::StartsWith => format!("{}%", escape_like(&self.value)),
                Operator::EndsWith => format!("%{}", escape_like(&self.value)),
                Operator::GreaterThan => return (format!("{column} > ?"), self.value.clone()),
                Operator::LessThan => return (format!("{column} < ?"), self.value.clone()),
            };
            let keyword = if self.operator == Operator::NotContains {
                "NOT LIKE"
            } else {
                "LIKE"
            };
            (format!("{column} {keyword} ? ESCAPE '\\'"), pattern)
        } else {
            let keyword = match self.operator {
                Operator::Equals
                | Operator::Contains
                | Operator::StartsWith
                | Operator::EndsWith => "=",
                Operator::NotEquals | Operator::NotContains => "!=",
                Operator::GreaterThan => ">",
                Operator::LessThan => "<",
            };
            (format!("{column} {keyword} ?"), self.value.clone())
        }
    }
}

fn escape_like(value: &str) -> String {
    value
        .replace('\\', "\\\\")
        .replace('%', "\\%")
        .replace('_', "\\_")
}

/// How multiple terms combine, mirroring `SmartPlaylistSearch::SearchType`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum Combine {
    /// All terms must match.
    #[default]
    And,
    /// Any term may match.
    Or,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn contains_uses_like_escape() {
        let term = SearchTerm {
            field: Field::Artist,
            operator: Operator::Contains,
            value: "100%".into(),
        };
        let (sql, param) = term.to_sql();
        assert_eq!(sql, "artist LIKE ? ESCAPE '\\'");
        assert_eq!(param, "%100\\%%");
    }

    #[test]
    fn numeric_operators() {
        let term = SearchTerm {
            field: Field::Rating,
            operator: Operator::GreaterThan,
            value: "4".into(),
        };
        assert_eq!(term.to_sql(), ("rating > ?".to_string(), "4".to_string()));
    }

    #[test]
    fn not_contains() {
        let term = SearchTerm {
            field: Field::Genre,
            operator: Operator::NotContains,
            value: "Pop".into(),
        };
        assert_eq!(term.to_sql().0, "genre NOT LIKE ? ESCAPE '\\'");
    }
}
