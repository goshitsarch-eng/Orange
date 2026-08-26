#include "dialogs/dialoglistkeyboard.h"
#include "dialogs/edittagfields.h"
#include "lyrics/lyricsproviderorder.h"
#include "widgets/listboxkeyboard.h"

#include <gtest/gtest.h>

namespace {

Song MakeTagged(const std::string &title, const std::string &artist, const std::string &album) {
  Song song(Song::Source::Collection);
  song.set_title(title);
  song.set_artist(artist);
  song.set_album(album);
  song.set_albumartist(artist);
  song.set_year(1994);
  song.set_track(8);
  song.set_playcount(4);
  song.set_skipcount(2);
  song.set_lastplayed(1700000000);
  song.set_url("file:///tmp/music/" + title + ".flac");
  song.set_valid(true);
  return song;
}

}  // namespace

TEST(EditTagFields, CommonValueDetectsMixedAndShared) {
  Song roads = MakeTagged("Roads", "Portishead", "Dummy");
  Song glory = MakeTagged("Glory Box", "Portishead", "Dummy");
  glory.set_year(1994);
  const auto artist = EditTagFields::CommonValue({roads, glory}, [](const Song &song) { return song.artist(); });
  EXPECT_EQ("Portishead", artist.first);
  EXPECT_FALSE(artist.second);
  const auto title = EditTagFields::CommonValue({roads, glory}, [](const Song &song) { return song.title(); });
  EXPECT_TRUE(title.first.empty());
  EXPECT_TRUE(title.second);
  const auto empty = EditTagFields::CommonValue({}, [](const Song &song) { return song.title(); });
  EXPECT_TRUE(empty.first.empty());
  EXPECT_FALSE(empty.second);
}

TEST(EditTagFields, ApplyFieldWritesTextAndNumericTags) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EditTagFields::ApplyField(&song, "Title", "Wandering Star");
  EditTagFields::ApplyField(&song, "Artist", "Portishead");
  EditTagFields::ApplyField(&song, "Album artist", "Portishead");
  EditTagFields::ApplyField(&song, "Year", "1994");
  EditTagFields::ApplyField(&song, "Track", "");
  EditTagFields::ApplyField(&song, "BPM", "120.5");
  EditTagFields::ApplyField(&song, "Comment", "remaster");
  EXPECT_EQ("Wandering Star", song.title());
  EXPECT_EQ("Portishead", song.albumartist());
  EXPECT_EQ(1994, song.year());
  EXPECT_EQ(-1, song.track());
  EXPECT_NEAR(120.5f, song.bpm(), 0.01f);
  EXPECT_EQ("remaster", song.comment());
}

TEST(EditTagFields, ApplyChangedFieldsUpdatesAllSongs) {
  SongList songs = {MakeTagged("Roads", "Portishead", "Dummy"), MakeTagged("Glory Box", "Portishead", "Dummy")};
  EditTagFields::ApplyChangedFields(&songs, {{"Genre", "Trip Hop"}, {"Year", "1994"}});
  EXPECT_EQ("Trip Hop", songs[0].genre());
  EXPECT_EQ("Trip Hop", songs[1].genre());
  EXPECT_EQ(1994, songs[1].year());
}

TEST(EditTagFields, ResetPlayStatisticsClearsCounts) {
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EditTagFields::ResetPlayStatistics(&song);
  EXPECT_EQ(0u, song.playcount());
  EXPECT_EQ(0u, song.skipcount());
  EXPECT_EQ(-1, song.lastplayed());
  SongList songs = {MakeTagged("A", "B", "C"), MakeTagged("D", "E", "F")};
  EditTagFields::ResetPlayStatistics(&songs);
  EXPECT_EQ(0u, songs[1].playcount());
  EXPECT_EQ(-1, songs[1].lastplayed());
}

TEST(EditTagFields, WrapIndexAndSongRowLabel) {
  EXPECT_EQ(0, EditTagFields::WrapIndex(0, 1, 0));
  EXPECT_EQ(0, EditTagFields::WrapIndex(2, 1, 3));
  EXPECT_EQ(2, EditTagFields::WrapIndex(0, -1, 3));
  Song song = MakeTagged("Roads", "Portishead", "Dummy");
  EXPECT_EQ("Portishead - Roads", EditTagFields::SongRowLabel(song));
}

TEST(DialogListKeyboard, ActivateMoveAndWrap) {
  EXPECT_TRUE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kReturn));
  EXPECT_TRUE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kKPEnter));
  EXPECT_FALSE(DialogListKeyboard::IsActivate(ListBoxKeyboard::kDown));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kUp));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kDown));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kHome));
  EXPECT_TRUE(DialogListKeyboard::IsMove(ListBoxKeyboard::kEnd));
  EXPECT_FALSE(DialogListKeyboard::IsMove(ListBoxKeyboard::kReturn));
  EXPECT_FALSE(DialogListKeyboard::IsMove(ListBoxKeyboard::kDelete));
  EXPECT_EQ(2, DialogListKeyboard::NextIndex(0, 3, ListBoxKeyboard::kUp));
  EXPECT_EQ(0, DialogListKeyboard::NextIndex(2, 3, ListBoxKeyboard::kDown));
  EXPECT_EQ(0, DialogListKeyboard::NextIndex(2, 3, ListBoxKeyboard::kHome));
  EXPECT_EQ(2, DialogListKeyboard::NextIndex(0, 3, ListBoxKeyboard::kEnd));
  EXPECT_EQ(-1, DialogListKeyboard::NextIndex(0, 0, ListBoxKeyboard::kDown));
}

TEST(LyricsProviderOrder, ParseJoinMoveAndRank) {
  const std::vector<std::string> parsed = LyricsProviderOrder::Parse(" LrcLib , Genius, ,OVH ");
  ASSERT_EQ(3u, parsed.size());
  EXPECT_EQ("LrcLib", parsed[0]);
  EXPECT_EQ("Genius", parsed[1]);
  EXPECT_EQ("OVH", parsed[2]);
  EXPECT_EQ("LrcLib,Genius,OVH", LyricsProviderOrder::Join(parsed));
  EXPECT_EQ(1, LyricsProviderOrder::Rank(parsed, "Genius", 99));
  EXPECT_EQ(99, LyricsProviderOrder::Rank(parsed, "Missing", 99));
  const std::vector<std::string> moved = LyricsProviderOrder::Move(parsed, 0, 1);
  ASSERT_EQ(3u, moved.size());
  EXPECT_EQ("Genius", moved[0]);
  EXPECT_EQ("LrcLib", moved[1]);
  EXPECT_EQ(parsed, LyricsProviderOrder::Move(parsed, 0, -1));
}
