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

TEST(SmartPlaylist, EmptyAndNotEmpty) {
  Song with_comment;
  with_comment.set_title("Has comment");
  with_comment.set_comment("live");
  Song empty_comment;
  empty_comment.set_title("No comment");
  SmartPlaylistTerm empty;
  empty.field = SmartPlaylistField::Comment;
  empty.op = SmartPlaylistOp::Empty;
  SmartPlaylistSearch search;
  search.terms.push_back(empty);
  const SongList missing = search.Search({with_comment, empty_comment});
  ASSERT_EQ(1u, missing.size());
  EXPECT_EQ("No comment", missing.front().title());

  empty.op = SmartPlaylistOp::NotEmpty;
  search.terms = {empty};
  const SongList present = search.Search({with_comment, empty_comment});
  ASSERT_EQ(1u, present.size());
  EXPECT_EQ("Has comment", present.front().title());
}

TEST(SmartPlaylist, AndOrTermsAndSort) {
  Song a;
  a.set_title("Alpha");
  a.set_artist("Fleet Foxes");
  a.set_year(2008);
  Song b;
  b.set_title("Beta");
  b.set_artist("Fleet Foxes");
  b.set_year(2011);
  Song c;
  c.set_title("Gamma");
  c.set_artist("The Shins");
  c.set_year(2001);

  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::And;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  search.terms.push_back({SmartPlaylistField::Year, SmartPlaylistOp::GreaterThan, "2009"});
  search.sort_field = SmartPlaylistField::Title;
  const SongList and_result = search.Search({a, b, c});
  ASSERT_EQ(1u, and_result.size());
  EXPECT_EQ("Beta", and_result.front().title());

  search.type = SmartPlaylistSearch::SearchType::Or;
  search.sort_field = SmartPlaylistField::Year;
  search.sort_descending = true;
  const SongList or_result = search.Search({a, b, c});
  ASSERT_EQ(2u, or_result.size());
  EXPECT_EQ("Beta", or_result.front().title());
  EXPECT_EQ("Alpha", or_result.back().title());
}

TEST(SmartPlaylist, ExtraFieldsAndLimit) {
  Song keyed;
  keyed.set_title("Key");
  keyed.set_initial_key("4A");
  keyed.set_track(2);
  keyed.set_originalyear(1999);
  Song other;
  other.set_title("Other");
  other.set_track(9);
  other.set_originalyear(2010);

  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::InitialKey, SmartPlaylistOp::Equals, "4A"});
  search.terms.push_back({SmartPlaylistField::OriginalYear, SmartPlaylistOp::LessThan, "2005"});
  const SongList result = search.Search({keyed, other});
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("Key", result.front().title());

  search.terms.clear();
  search.sort_field = SmartPlaylistField::Track;
  search.sort_descending = true;
  search.limit = 1;
  const SongList limited = search.Search({keyed, other});
  ASSERT_EQ(1u, limited.size());
  EXPECT_EQ("Other", limited.front().title());
}

TEST(SmartPlaylist, SerializeRoundTrip) {
  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::Or;
  search.limit = 25;
  search.sort_field = SmartPlaylistField::Year;
  search.sort_descending = true;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  search.terms.push_back({SmartPlaylistField::Comment, SmartPlaylistOp::Empty, {}});
  const std::string blob = search.Serialize();
  SmartPlaylistSearch parsed;
  ASSERT_TRUE(SmartPlaylistSearch::Parse(blob, &parsed));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::Or, parsed.type);
  EXPECT_EQ(25, parsed.limit);
  EXPECT_EQ(SmartPlaylistField::Year, parsed.sort_field);
  EXPECT_TRUE(parsed.sort_descending);
  ASSERT_EQ(2u, parsed.terms.size());
  EXPECT_EQ(SmartPlaylistField::Artist, parsed.terms[0].field);
  EXPECT_EQ(SmartPlaylistOp::Contains, parsed.terms[0].op);
  EXPECT_EQ("fleet", parsed.terms[0].value);
  EXPECT_EQ(SmartPlaylistOp::Empty, parsed.terms[1].op);

  SmartPlaylistSearch::AddSaved("Fleet years", search);
  const auto saved = SmartPlaylistSearch::LoadSaved();
  bool found = false;
  for (const auto &preset : saved) {
    if (preset.first == "Fleet years") {
      found = true;
      EXPECT_EQ(25, preset.second.limit);
    }
  }
  EXPECT_TRUE(found);
  SmartPlaylistSearch::RemoveSaved("Fleet years");
}

TEST(SmartPlaylist, FieldAndOpIndex) {
  EXPECT_EQ(SmartPlaylistField::Title, SmartPlaylistSearch::FieldFromIndex(0));
  EXPECT_EQ(SmartPlaylistField::InitialKey, SmartPlaylistSearch::FieldFromIndex(static_cast<int>(SmartPlaylistSearch::FieldNames().size()) - 1));
  EXPECT_EQ(SmartPlaylistOp::Contains, SmartPlaylistSearch::OpFromIndex(0));
  EXPECT_EQ(SmartPlaylistOp::NotEmpty, SmartPlaylistSearch::OpFromIndex(static_cast<int>(SmartPlaylistSearch::OpNames().size()) - 1));
  EXPECT_EQ(SmartPlaylistSearch::FieldNames().size(), 29u);
  EXPECT_EQ(SmartPlaylistSearch::OpNames().size(), 10u);
}
