#include "core/song.h"

#include <gtest/gtest.h>

TEST(Song, Defaults) {
  Song song;
  EXPECT_FALSE(song.is_valid());
  EXPECT_EQ(-1, song.id());
  EXPECT_EQ(Song::Source::Unknown, song.source());
}

TEST(Song, PrettyTitle) {
  Song song;
  song.set_title("Helplessness Blues");
  song.set_artist("Fleet Foxes");
  EXPECT_EQ("Helplessness Blues", song.PrettyTitle());
  EXPECT_EQ("Fleet Foxes - Helplessness Blues", song.PrettyTitleWithArtist());
}

TEST(Song, FiletypeAndAudio) {
  EXPECT_EQ(Song::FileType::FLAC, Song::FiletypeByExtension("flac"));
  EXPECT_EQ(Song::FileType::FLAC, Song::FiletypeByMimeType("audio/flac"));
  EXPECT_EQ(Song::FileType::MPEG, Song::FiletypeByFilename("track.mp3"));
  EXPECT_TRUE(Song::IsAudioFile("album/01.flac"));
  EXPECT_FALSE(Song::IsAudioFile("notes.txt"));
  EXPECT_EQ("FLAC", Song::FiletypeToString(Song::FileType::FLAC));
  EXPECT_EQ("MP3", Song::FiletypeToString(Song::FileType::MPEG));
  EXPECT_EQ("Collection", Song::SourceToString(Song::Source::Collection));
}

TEST(Song, AlbumRemoveDiscMisc) {
  EXPECT_EQ("Dummy", Song::AlbumRemoveDiscMisc("Dummy - Disc 2"));
  EXPECT_EQ("Dummy", Song::AlbumRemoveDiscMisc("Dummy (CD 1)"));
  EXPECT_EQ("Dummy", Song::AlbumRemoveDiscMisc("Dummy [Remastered]"));
  EXPECT_EQ("Dummy", Song::AlbumRemoveDiscMisc("Dummy (Explicit)"));
}

TEST(Song, EqualityByUrl) {
  Song a;
  a.set_url("file:///tmp/a.flac");
  Song b;
  b.set_url("file:///tmp/a.flac");
  EXPECT_EQ(a, b);
}
