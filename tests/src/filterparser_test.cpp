#include "filterparser/filterparser.h"

#include <sqlite3.h>

#include <ctime>
#include <string>

#include <gtest/gtest.h>

TEST(FilterParser, FreeText) {
  Song song;
  song.set_title("Helplessness Blues");
  song.set_artist("Fleet Foxes");
  EXPECT_TRUE(FilterParser("fleet").Matches(song));
  EXPECT_FALSE(FilterParser("radiohead").Matches(song));
}

TEST(FilterParser, FieldAndNegation) {
  Song song;
  song.set_title("Ragged Wood");
  song.set_genre("Folk");
  EXPECT_TRUE(FilterParser("genre:Folk").Matches(song));
  EXPECT_FALSE(FilterParser("-title:Ragged").Matches(song));
}

TEST(FilterParser, AgeAndLastPlayed) {
  Song recent;
  recent.set_title("New");
  recent.set_ctime(static_cast<int64_t>(std::time(nullptr)) - 3600);
  recent.set_lastplayed(static_cast<int64_t>(std::time(nullptr)) - 1800);
  Song old;
  old.set_title("Old");
  old.set_ctime(static_cast<int64_t>(std::time(nullptr)) - 40 * 86400);
  old.set_lastplayed(-1);
  EXPECT_TRUE(FilterParser("age:7").Matches(recent));
  EXPECT_FALSE(FilterParser("age:7").Matches(old));
  EXPECT_TRUE(FilterParser("lastplayed:1").Matches(recent));
  EXPECT_FALSE(FilterParser("lastplayed:1").Matches(old));
}

TEST(FilterParser, NumericOperators) {
  Song song;
  song.set_title("Track");
  song.set_year(2011);
  song.set_rating(0.8f);
  song.set_playcount(12);
  song.set_bitrate(320);
  EXPECT_TRUE(FilterParser("year:>=2010").Matches(song));
  EXPECT_TRUE(FilterParser("year>=2010").Matches(song));
  EXPECT_FALSE(FilterParser("year:<2010").Matches(song));
  EXPECT_TRUE(FilterParser("rating:>=0.5").Matches(song));
  EXPECT_TRUE(FilterParser("playcount:>10").Matches(song));
  EXPECT_TRUE(FilterParser("bitrate:=320").Matches(song));
}

TEST(FilterParser, QuotesAndOrNot) {
  Song foxes;
  foxes.set_title("Helplessness Blues");
  foxes.set_artist("Fleet Foxes");
  foxes.set_genre("Folk");
  Song radio;
  radio.set_title("Weird Fishes");
  radio.set_artist("Radiohead");
  radio.set_genre("Rock");
  EXPECT_TRUE(FilterParser("title:\"Helplessness Blues\"").Matches(foxes));
  EXPECT_TRUE(FilterParser("artist:Fleet OR artist:Radiohead").Matches(foxes));
  EXPECT_TRUE(FilterParser("artist:Fleet OR artist:Radiohead").Matches(radio));
  EXPECT_TRUE(FilterParser("genre:Folk AND -artist:Radiohead").Matches(foxes));
  EXPECT_FALSE(FilterParser("genre:Folk AND -artist:Fleet").Matches(foxes));
}

TEST(FilterParser, ToSqlContainsColumnsAndOperators) {
  const std::string sql = FilterParser("genre:Folk year:>=2010 -title:Demo").ToSql();
  EXPECT_NE(std::string::npos, sql.find("genre"));
  EXPECT_NE(std::string::npos, sql.find("year"));
  EXPECT_NE(std::string::npos, sql.find(">="));
  EXPECT_NE(std::string::npos, sql.find("NOT"));
}

TEST(FilterParser, BuildsFilterTree) {
  FilterParser parser("genre:Folk OR artist:Fleet");
  FilterTree *tree = parser.parse();
  ASSERT_NE(nullptr, tree);
  EXPECT_EQ(FilterTree::FilterType::Or, tree->type());
  Song folk;
  folk.set_genre("Folk");
  folk.set_artist("Other");
  EXPECT_TRUE(tree->accept(folk));
  EXPECT_FALSE(FilterParser::ToolTip().empty());
  EXPECT_NE(std::string::npos, FilterParser::ToolTip().find("artist:Strawbs"));
  EXPECT_NE(std::string::npos, FilterParser::ToolTip().find("AND"));
  EXPECT_NE(std::string::npos, FilterParser::ToolTip().find("OR"));
  EXPECT_NE(std::string::npos, FilterParser::ToolTip().find("rating"));
  EXPECT_NE(std::string::npos, FilterParser::ToolTip().find("title, album, artist"));
}

// The generated SQL is spliced straight into a WHERE clause, so the real property to hold is that whatever
// the user typed comes back out as data. SQLite itself is the authority on that, so ask it: build the same
// statement the collection backend builds and check it parses.
namespace {

bool WhereClauseParses(const std::string &filter, std::string *sql_out) {
  sqlite3 *db = nullptr;
  if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
    return false;
  }
  const char *schema =
      "CREATE TABLE songs (title TEXT, titlesort TEXT, album TEXT, albumsort TEXT, artist TEXT, artistsort TEXT, "
      "albumartist TEXT, albumartistsort TEXT, composer TEXT, performer TEXT, grouping TEXT, genre TEXT, comment TEXT, "
      "url TEXT, filetype TEXT, track INTEGER, disc INTEGER, year INTEGER, originalyear INTEGER, length INTEGER, "
      "samplerate INTEGER, bitdepth INTEGER, bitrate INTEGER, playcount INTEGER, skipcount INTEGER, rating INTEGER, "
      "ctime INTEGER, mtime INTEGER, lastplayed INTEGER, filesize INTEGER)";
  if (sqlite3_exec(db, schema, nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_close(db);
    return false;
  }
  const std::string where = FilterParser(filter).ToSql();
  if (sql_out) {
    *sql_out = where;
  }
  const std::string sql = "SELECT title FROM songs WHERE " + where;
  sqlite3_stmt *stmt = nullptr;
  const int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
  const bool ok = rc == SQLITE_OK;
  if (stmt) {
    sqlite3_finalize(stmt);
  }
  sqlite3_close(db);
  return ok;
}

}  // namespace

TEST(FilterParser, ApostropheInFreeTextProducesValidSql) {
  std::string sql;
  EXPECT_TRUE(WhereClauseParses("Guns N' Roses", &sql)) << sql;
}

TEST(FilterParser, ApostropheInFieldValueProducesValidSql) {
  std::string sql;
  EXPECT_TRUE(WhereClauseParses("artist:O'Connor", &sql)) << sql;
  // The apostrophe has to be doubled rather than passed through, or it would close the literal.
  EXPECT_NE(sql.find("O''Connor"), std::string::npos) << sql;
}

TEST(FilterParser, InjectionAttemptStaysInsideTheLiteral) {
  std::string sql;
  EXPECT_TRUE(WhereClauseParses("' OR 1=1 --", &sql)) << sql;
  EXPECT_TRUE(WhereClauseParses("artist:x' OR '1'='1", &sql)) << sql;
  // "OR '1'='1" must survive only as escaped text inside the LIKE pattern, never as its own clause.
  EXPECT_EQ(sql.find("OR '1'='1"), std::string::npos) << sql;
}

TEST(FilterParser, NonNumericValueForNumericColumnIsRejected) {
  std::string sql;
  EXPECT_TRUE(WhereClauseParses("year:>0)OR(1=1", &sql)) << sql;
  EXPECT_EQ(sql.find("OR(1=1"), std::string::npos) << sql;

  // A bare word would otherwise be resolved by SQLite as a column name rather than rejected.
  EXPECT_TRUE(WhereClauseParses("year:>abc", &sql)) << sql;
  EXPECT_EQ(sql.find("abc"), std::string::npos) << sql;
}

TEST(FilterParser, NumericValueStillReachesTheSql) {
  std::string sql;
  EXPECT_TRUE(WhereClauseParses("year:>2000", &sql)) << sql;
  EXPECT_NE(sql.find("2000"), std::string::npos) << sql;
}

// Matching must keep working for the values that used to break the SQL.
TEST(FilterParser, ApostropheStillMatches) {
  Song song;
  song.set_title("Livin' on a Prayer");
  song.set_artist("Bon Jovi");
  EXPECT_TRUE(FilterParser("Livin'").Matches(song));
  EXPECT_FALSE(FilterParser("Dyin'").Matches(song));
}
