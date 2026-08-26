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
