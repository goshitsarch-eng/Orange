#include "smartplaylists/smartplaylist.h"

#include <gtest/gtest.h>

TEST(SmartPlaylist, ArtistContains) {
  Song match;
  match.set_artist("Fleet Foxes");
  match.set_title("Ragged Wood");
  match.set_valid(true);
  Song other;
  other.set_artist("The Shins");
  other.set_title("New Slang");
  other.set_valid(true);

  SmartPlaylistTerm term;
  term.field = SmartPlaylistField::Artist;
  term.op = SmartPlaylistOp::Contains;
  term.value = "fleet";
  SmartPlaylistSearch search;
  search.terms.push_back(term);
  const SongList result = search.Search({match, other});
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("Ragged Wood", result.front().title());
}
