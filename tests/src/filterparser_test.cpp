#include "filterparser/filterparser.h"

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
