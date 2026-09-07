//! SQL query builder for the `songs` table.
//! Mirrors `CollectionQuery`: same table/column names, same join-free
//! structure, bound parameters instead of string interpolation.

/// Sort column for collection results.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SortColumn {
    Artist,
    Album,
    Title,
    Track,
    Year,
    Genre,
    Rating,
    PlayCount,
    LastPlayed,
}

impl SortColumn {
    fn column(self) -> &'static str {
        match self {
            Self::Artist => "artist",
            Self::Album => "album",
            Self::Title => "title",
            Self::Track => "track",
            Self::Year => "year",
            Self::Genre => "genre",
            Self::Rating => "rating",
            Self::PlayCount => "playcount",
            Self::LastPlayed => "lastplayed",
        }
    }
}

/// Optional constraints on a collection query. Unset fields are unfiltered.
#[derive(Debug, Clone, Default)]
pub struct CollectionQuery {
    pub artist: Option<String>,
    pub album: String,
    pub albumartist: Option<String>,
    pub genre: Option<String>,
    pub year_min: Option<i64>,
    pub year_max: Option<i64>,
    pub min_rating: Option<f64>,
    pub min_playcount: Option<i64>,
    pub only_available: bool,
    pub sort: Option<(SortColumn, bool)>,
    pub limit: Option<i64>,
}

impl CollectionQuery {
    /// Build `(sql, params)`. Placeholders are `?` in order; the caller binds
    /// `params` positionally, exactly like the 2.1.5 query path.
    pub fn build(&self) -> (String, Vec<QueryParam>) {
        let mut sql = String::from("SELECT * FROM songs WHERE 1=1");
        let mut params = Vec::new();
        if let Some(artist) = &self.artist {
            sql.push_str(" AND artist = ?");
            params.push(QueryParam::Text(artist.clone()));
        }
        if !self.album.is_empty() {
            // No-op guard kept explicit: empty album means "no album filter".
        }
        if let Some(albumartist) = &self.albumartist {
            sql.push_str(" AND albumartist = ?");
            params.push(QueryParam::Text(albumartist.clone()));
        }
        if let Some(genre) = &self.genre {
            sql.push_str(" AND genre = ?");
            params.push(QueryParam::Text(genre.clone()));
        }
        if let Some(min) = self.year_min {
            sql.push_str(" AND year >= ?");
            params.push(QueryParam::Int(min));
        }
        if let Some(max) = self.year_max {
            sql.push_str(" AND year <= ?");
            params.push(QueryParam::Int(max));
        }
        if let Some(rating) = self.min_rating {
            sql.push_str(" AND rating >= ?");
            params.push(QueryParam::Real(rating));
        }
        if let Some(playcount) = self.min_playcount {
            sql.push_str(" AND playcount >= ?");
            params.push(QueryParam::Int(playcount));
        }
        if self.only_available {
            sql.push_str(" AND unavailable = 0");
        }
        if let Some((column, descending)) = self.sort {
            sql.push_str(" ORDER BY ");
            sql.push_str(column.column());
            sql.push_str(if descending { " DESC" } else { " ASC" });
        }
        if let Some(limit) = self.limit {
            sql.push_str(" LIMIT ?");
            params.push(QueryParam::Int(limit));
        }
        (sql, params)
    }
}

/// A bound query parameter.
#[derive(Debug, Clone, PartialEq)]
pub enum QueryParam {
    Text(String),
    Int(i64),
    Real(f64),
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn empty_query_selects_everything() {
        let (sql, params) = CollectionQuery::default().build();
        assert_eq!(sql, "SELECT * FROM songs WHERE 1=1");
        assert!(params.is_empty());
    }

    #[test]
    fn filters_bind_in_order() {
        let q = CollectionQuery {
            artist: Some("Miles Davis".to_string()),
            year_min: Some(1959),
            year_max: Some(1959),
            only_available: true,
            sort: Some((SortColumn::Year, false)),
            limit: Some(50),
            ..Default::default()
        };
        let (sql, params) = q.build();
        assert_eq!(
            sql,
            "SELECT * FROM songs WHERE 1=1 AND artist = ? AND year >= ? AND year <= ? \
       AND unavailable = 0 ORDER BY year ASC LIMIT ?"
        );
        assert_eq!(
            params,
            vec![
                QueryParam::Text("Miles Davis".to_string()),
                QueryParam::Int(1959),
                QueryParam::Int(1959),
                QueryParam::Int(50),
            ]
        );
    }

    #[test]
    fn empty_album_is_no_filter() {
        let q = CollectionQuery {
            album: String::new(),
            ..Default::default()
        };
        assert_eq!(q.build().0, "SELECT * FROM songs WHERE 1=1");
    }
}
