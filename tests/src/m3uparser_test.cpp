#include "playlistparsers/playlistparser.h"
#include "utilities/fileutils.h"

#include <gtest/gtest.h>

#include <unistd.h>

TEST(PlaylistParser, RoundTripM3U) {
  const std::string path = "/tmp/strawberry-test-" + std::to_string(getpid()) + ".m3u";
  Song song;
  song.set_title("White Winter Hymnal");
  song.set_artist("Fleet Foxes");
  song.set_url("file:///tmp/white-winter-hymnal.flac");
  song.set_length_nanosec(27000000000LL);
  song.set_valid(true);
  PlaylistParser parser;
  ASSERT_TRUE(parser.Save(path, {song}));
  const SongList loaded = parser.Load(path);
  ASSERT_FALSE(loaded.empty());
  EXPECT_FALSE(loaded.front().url().empty());
  unlink(path.c_str());
}

TEST(PlaylistParser, DetectsExtensions) {
  EXPECT_TRUE(PlaylistParser::IsPlaylist("list.m3u"));
  EXPECT_TRUE(PlaylistParser::IsPlaylist("list.xspf"));
  EXPECT_TRUE(PlaylistParser::IsPlaylist("album.cue"));
  EXPECT_FALSE(PlaylistParser::IsPlaylist("song.flac"));
}

TEST(PlaylistParser, CueIndexAndTracks) {
  EXPECT_EQ(0, PlaylistParser::CueIndexToNanosec("00:00:00"));
  EXPECT_EQ(2000000000LL, PlaylistParser::CueIndexToNanosec("00:02:00"));
  const std::string path = "/tmp/strawberry-test-" + std::to_string(getpid()) + ".cue";
  const std::string cue =
      "PERFORMER \"Artist\"\n"
      "TITLE \"Album\"\n"
      "FILE \"album.flac\" WAVE\n"
      "  TRACK 01 AUDIO\n"
      "    TITLE \"One\"\n"
      "    PERFORMER \"Artist\"\n"
      "    INDEX 01 00:00:00\n"
      "  TRACK 02 AUDIO\n"
      "    TITLE \"Two\"\n"
      "    INDEX 01 00:02:00\n";
  ASSERT_TRUE(FileUtils::WriteFile(path, cue));
  const SongList songs = PlaylistParser().Load(path);
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ("One", songs[0].title());
  EXPECT_EQ("Album", songs[0].album());
  EXPECT_EQ("Artist", songs[0].artist());
  EXPECT_EQ(0, songs[0].beginning_nanosec());
  EXPECT_EQ(2000000000LL, songs[0].length_nanosec());
  EXPECT_EQ("Two", songs[1].title());
  EXPECT_EQ(2000000000LL, songs[1].beginning_nanosec());
  EXPECT_FALSE(songs[0].cue_path().empty());
  unlink(path.c_str());
}
