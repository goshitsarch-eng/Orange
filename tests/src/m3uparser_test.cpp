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
  EXPECT_FALSE(PlaylistParser::IsPlaylist("song.flac"));
}
