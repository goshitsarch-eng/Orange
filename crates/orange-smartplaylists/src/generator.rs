//! Smart playlist query generation: terms + ordering + limit become the
//! exact SQL the collection backend runs. Mirrors `QueryGenerator`.

use crate::search::{Combine, SearchTerm};

/// Sort order for generated playlists.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SortOrder {
    Random,
    Newest,
    Oldest,
    HighestRated,
    MostPlayed,
}

/// A complete smart playlist definition.
#[derive(Debug, Clone, Default)]
pub struct SmartPlaylist {
    pub terms: Vec<SearchTerm>,
    pub combine: Combine,
    pub limit: Option<i64>,
    pub sort: Option<SortOrder>,
}

/// Generated SQL plus bound values in placeholder order.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct GeneratedQuery {
    pub sql: String,
    pub params: Vec<String>,
}

impl SmartPlaylist {
    pub fn generate(&self) -> GeneratedQuery {
        let mut sql = String::from("SELECT * FROM songs");
        let mut params = Vec::new();
        if !self.terms.is_empty() {
            let joiner = match self.combine {
                Combine::And => " AND ",
                Combine::Or => " OR ",
            };
            let mut first = true;
            sql.push_str(" WHERE ");
            for term in &self.terms {
                if !first {
                    sql.push_str(joiner);
                }
                first = false;
                let (fragment, param) = term.to_sql();
                sql.push_str(&fragment);
                params.push(param);
            }
        }
        match self.sort {
            Some(SortOrder::Random) => sql.push_str(" ORDER BY RANDOM()"),
            Some(SortOrder::Newest) => sql.push_str(" ORDER BY lastplayed DESC"),
            Some(SortOrder::Oldest) => sql.push_str(" ORDER BY lastplayed ASC"),
            Some(SortOrder::HighestRated) => sql.push_str(" ORDER BY rating DESC"),
            Some(SortOrder::MostPlayed) => sql.push_str(" ORDER BY playcount DESC"),
            None => {}
        }
        if let Some(limit) = self.limit {
            sql.push_str(" LIMIT ");
            sql.push_str(&limit.to_string());
        }
        GeneratedQuery { sql, params }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::search::{Field, Operator};

    fn term(field: Field, operator: Operator, value: &str) -> SearchTerm {
        SearchTerm {
            field,
            operator,
            value: value.to_string(),
        }
    }

    #[test]
    fn golden_query_and_limit() {
        let list = SmartPlaylist {
            terms: vec![
                term(Field::Genre, Operator::Equals, "Jazz"),
                term(Field::Rating, Operator::GreaterThan, "3"),
            ],
            combine: Combine::And,
            limit: Some(25),
            sort: Some(SortOrder::HighestRated),
        };
        let query = list.generate();
        assert_eq!(
            query.sql,
            "SELECT * FROM songs WHERE genre = ? AND rating > ? ORDER BY rating DESC LIMIT 25"
        );
        assert_eq!(query.params, vec!["Jazz".to_string(), "3".to_string()]);
    }

    #[test]
    fn or_combine_and_random() {
        let list = SmartPlaylist {
            terms: vec![
                term(Field::Artist, Operator::Contains, "Miles"),
                term(Field::Artist, Operator::Contains, "Coltrane"),
            ],
            combine: Combine::Or,
            limit: None,
            sort: Some(SortOrder::Random),
        };
        let query = list.generate();
        assert_eq!(
            query.sql,
            "SELECT * FROM songs WHERE artist LIKE ? ESCAPE '\\' OR artist LIKE ? ESCAPE '\\' \
       ORDER BY RANDOM()"
        );
        assert_eq!(
            query.params,
            vec!["%Miles%".to_string(), "%Coltrane%".to_string()]
        );
    }

    #[test]
    fn empty_terms_select_all() {
        let query = SmartPlaylist::default().generate();
        assert_eq!(query.sql, "SELECT * FROM songs");
        assert!(query.params.is_empty());
    }
}
