#include "config.h"
#include "device/gpodcover.h"
#include "device/gpoddelete.h"
#include "device/gpoddevice.h"
#include "device/gpodloader.h"

#include <gtest/gtest.h>

#ifdef HAVE_GPOD

TEST(GPodLoader, SongFromTrackRoundTrip) {
  Itdb_Track *track = itdb_track_new();
  track->title = g_strdup("Helplessness Blues");
  track->artist = g_strdup("Fleet Foxes");
  track->album = g_strdup("Helplessness Blues");
  track->albumartist = g_strdup("Fleet Foxes");
  track->track_nr = 2;
  track->cd_nr = 1;
  track->year = 2011;
  track->genre = g_strdup("Folk");
  track->tracklen = 301000;
  track->bitrate = 256;
  track->samplerate = 44100;
  track->type2 = 1;
  track->ipod_path = g_strdup(":iPod_Control:Music:F00:song.mp3");
  track->size = 12345;
  track->playcount = 4;
  track->skipcount = 1;

  const Song song = GPodLoader::SongFromTrack(track, "/media/ipod");
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ(Song::Source::Device, song.source());
  EXPECT_EQ("Helplessness Blues", song.title());
  EXPECT_EQ("Fleet Foxes", song.artist());
  EXPECT_EQ("Helplessness Blues", song.album());
  EXPECT_EQ(2, song.track());
  EXPECT_EQ(2011, song.year());
  EXPECT_EQ("Folk", song.genre());
  EXPECT_EQ(301000000000LL, song.length_nanosec());
  EXPECT_EQ(Song::FileType::MPEG, song.filetype());
  EXPECT_NE(std::string::npos, song.url().find("iPod_Control/Music/F00/song.mp3"));

  Itdb_Track *written = itdb_track_new();
  GPodLoader::SongToTrack(song, written);
  EXPECT_STREQ("Helplessness Blues", written->title);
  EXPECT_STREQ("Fleet Foxes", written->artist);
  EXPECT_EQ(2, written->track_nr);
  EXPECT_EQ(301000, written->tracklen);
  EXPECT_EQ(1, written->type2);

  itdb_track_free(track);
  itdb_track_free(written);
}

TEST(GPodLoader, LoadSongsMissingDatabase) {
  EXPECT_TRUE(GPodLoader::LoadSongs("/tmp/not-an-ipod").empty());
}

TEST(GPodCover, ShouldSetThumbnailsMatchesQt) {
  EXPECT_FALSE(GPodCover::ShouldSetThumbnails(false, "/tmp/cover.jpg"));
  EXPECT_FALSE(GPodCover::ShouldSetThumbnails(true, {}));
  EXPECT_TRUE(GPodCover::ShouldSetThumbnails(true, "/tmp/cover.jpg"));
}

TEST(GPodDelete, IpodPathFromUrlMatchesQtRemoveTrack) {
  EXPECT_EQ(":iPod_Control:Music:F00:song.mp3",
            GPodDelete::IpodPathFromUrl("file:///media/ipod/iPod_Control/Music/F00/song.mp3", "/media/ipod"));
  EXPECT_EQ(":iPod_Control:Music:F00:song.mp3",
            GPodDelete::IpodPathFromUrl("/media/ipod/iPod_Control/Music/F00/song.mp3", "/media/ipod"));
  EXPECT_EQ(":iPod_Control:Music:F00:song.mp3",
            GPodDelete::IpodPathFromUrl("/media/ipod/iPod_Control/Music/F00/song.mp3", "/media/ipod/"));
  EXPECT_TRUE(GPodDelete::TrackMatches(":iPod_Control:Music:F00:song.mp3", ":iPod_Control:Music:F00:song.mp3"));
  EXPECT_FALSE(GPodDelete::TrackMatches(nullptr, ":iPod_Control:Music:F00:song.mp3"));
  Song missing;
  missing.set_url("file:///tmp/not-an-ipod/iPod_Control/Music/F00/song.mp3");
  EXPECT_FALSE(GPodDevice::DeleteSong("/tmp/not-an-ipod", missing));
}

#else

TEST(GPodLoader, DisabledWithoutLibgpod) {
  EXPECT_TRUE(GPodLoader::LoadSongs("/tmp/not-an-ipod").empty());
}

#endif
