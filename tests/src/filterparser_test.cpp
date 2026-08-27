#include "filterparser/filterparser.h"

#include <ctime>
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
