#include "config.h"
#include "core/database.h"
#include "core/song.h"
#include "device/devicedatabasebackend.h"
#include "device/devicecopysupported.h"
#include "device/mtpconnection.h"
#include "device/mtploader.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <string>
#include <vector>

#ifdef HAVE_MTP

TEST(MtpConnection, ErrorStringMatchesOriginal) {
  EXPECT_EQ("No Devices have been found.", MtpConnection::ErrorString(LIBMTP_ERROR_NO_DEVICE_ATTACHED));
  EXPECT_EQ("There has been an error connecting.", MtpConnection::ErrorString(LIBMTP_ERROR_CONNECTING));
  EXPECT_EQ("Memory Allocation Error.", MtpConnection::ErrorString(LIBMTP_ERROR_MEMORY_ALLOCATION));
  EXPECT_EQ("Successfully connected.", MtpConnection::ErrorString(LIBMTP_ERROR_NONE));
  EXPECT_EQ("Unknown error, please report this to the libmtp developers.", MtpConnection::ErrorString(LIBMTP_ERROR_GENERAL));
}

TEST(MtpConnection, FileTypeFromMtpMatchesInitFromMTP) {
  EXPECT_EQ(Song::FileType::WAV, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_WAV));
  EXPECT_EQ(Song::FileType::MPEG, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_MP2));
  EXPECT_EQ(Song::FileType::MPEG, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_MP3));
  EXPECT_EQ(Song::FileType::ASF, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_WMA));
  EXPECT_EQ(Song::FileType::MP4, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_MP4));
  EXPECT_EQ(Song::FileType::MP4, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_M4A));
  EXPECT_EQ(Song::FileType::MP4, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_AAC));
  EXPECT_EQ(Song::FileType::OggFlac, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_FLAC));
  EXPECT_EQ(Song::FileType::OggVorbis, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_OGG));
  EXPECT_EQ(Song::FileType::Unknown, MtpConnection::FileTypeFromMtp(LIBMTP_FILETYPE_UNKNOWN));
}

TEST(DeviceCopySupported, AppendFromMtpMatchesQtGetSupportedFiletypes) {
  std::vector<Song::FileType> types;
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_WAV, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_MP2, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_MP3, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_WMA, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_MP4, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_M4A, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_AAC, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_FLAC, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_OGG, &types);
  DeviceCopySupported::AppendFromMtp(LIBMTP_FILETYPE_UNKNOWN, &types);
  ASSERT_EQ(12u, types.size());
  EXPECT_EQ(Song::FileType::WAV, types[0]);
  EXPECT_EQ(Song::FileType::MPEG, types[1]);
  EXPECT_EQ(Song::FileType::MPEG, types[2]);
  EXPECT_EQ(Song::FileType::ASF, types[3]);
  EXPECT_EQ(Song::FileType::MP4, types[4]);
  EXPECT_EQ(Song::FileType::MP4, types[5]);
  EXPECT_EQ(Song::FileType::MP4, types[6]);
  EXPECT_EQ(Song::FileType::FLAC, types[7]);
  EXPECT_EQ(Song::FileType::OggFlac, types[8]);
  EXPECT_EQ(Song::FileType::OggVorbis, types[9]);
  EXPECT_EQ(Song::FileType::OggSpeex, types[10]);
  EXPECT_EQ(Song::FileType::OggFlac, types[11]);
}

TEST(MtpConnection, MtpFileTypeFromSong) {
  EXPECT_EQ(LIBMTP_FILETYPE_MP3, MtpConnection::MtpFileTypeFromSong(Song::FileType::MPEG));
  EXPECT_EQ(LIBMTP_FILETYPE_FLAC, MtpConnection::MtpFileTypeFromSong(Song::FileType::FLAC));
  EXPECT_EQ(LIBMTP_FILETYPE_FLAC, MtpConnection::MtpFileTypeFromSong(Song::FileType::OggFlac));
  EXPECT_EQ(LIBMTP_FILETYPE_OGG, MtpConnection::MtpFileTypeFromSong(Song::FileType::OggVorbis));
  EXPECT_EQ(LIBMTP_FILETYPE_UNDEF_AUDIO, MtpConnection::MtpFileTypeFromSong(Song::FileType::Unknown));
}

TEST(MtpLoader, SongFromTrackRoundTrip) {
  LIBMTP_track_t track{};
  track.item_id = 42;
  track.title = const_cast<char *>("Helplessness Blues");
  track.artist = const_cast<char *>("Fleet Foxes");
  track.album = const_cast<char *>("Helplessness Blues");
  track.genre = const_cast<char *>("Folk");
  track.composer = const_cast<char *>("Robin Pecknold");
  track.tracknumber = 2;
  track.filename = const_cast<char *>("song.mp3");
  track.filesize = 12345;
  track.modificationdate = 1310000000;
  track.duration = 301000;
  track.bitrate = 256;
  track.samplerate = 44100;
  track.usecount = 4;
  track.filetype = LIBMTP_FILETYPE_MP3;

  const Song song = MtpLoader::SongFromTrack(&track, "ABC123");
  EXPECT_TRUE(song.is_valid());
  EXPECT_EQ(Song::Source::Device, song.source());
  EXPECT_EQ("Helplessness Blues", song.title());
  EXPECT_EQ("Fleet Foxes", song.artist());
  EXPECT_EQ("Helplessness Blues", song.album());
  EXPECT_EQ("Folk", song.genre());
  EXPECT_EQ("Robin Pecknold", song.composer());
  EXPECT_EQ(2, song.track());
  EXPECT_EQ("mtp://ABC123/42", song.url());
  EXPECT_EQ("42", song.basefilename());
  EXPECT_EQ(12345, song.filesize());
  EXPECT_EQ(301000000000LL, song.length_nanosec());
  EXPECT_EQ(256, song.bitrate());
  EXPECT_EQ(44100, song.samplerate());
  EXPECT_EQ(4u, song.playcount());
  EXPECT_EQ(Song::FileType::MPEG, song.filetype());

  LIBMTP_track_t *written = LIBMTP_new_track_t();
  ASSERT_TRUE(written);
  Song outgoing = song;
  outgoing.set_albumartist("Fleet Foxes");
  outgoing.set_basefilename("song.mp3");
  MtpLoader::SongToTrack(outgoing, written);
  EXPECT_STREQ("Helplessness Blues", written->title);
  EXPECT_STREQ("Fleet Foxes", written->artist);
  EXPECT_STREQ("Helplessness Blues", written->album);
  EXPECT_STREQ("Folk", written->genre);
  EXPECT_STREQ("Robin Pecknold", written->composer);
  EXPECT_STREQ("song.mp3", written->filename);
  EXPECT_EQ(2, written->tracknumber);
  EXPECT_EQ(301000u, written->duration);
  EXPECT_EQ(256, written->bitrate);
  EXPECT_EQ(44100u, written->samplerate);
  EXPECT_EQ(4u, written->usecount);
  EXPECT_EQ(LIBMTP_FILETYPE_MP3, written->filetype);
  LIBMTP_destroy_track_t(written);
}

TEST(MtpLoader, UnknownFiletypeIsInvalidUnlessFilenameFallback) {
  LIBMTP_track_t track{};
  track.item_id = 7;
  track.title = const_cast<char *>("Untitled");
  track.filetype = LIBMTP_FILETYPE_UNKNOWN;
  const Song invalid = MtpLoader::SongFromTrack(&track, "host");
  EXPECT_FALSE(invalid.is_valid());
  EXPECT_EQ(Song::FileType::Unknown, invalid.filetype());

  track.title = nullptr;
  track.filename = const_cast<char *>("fallback.wav");
  const Song recovered = MtpLoader::SongFromTrack(&track, "host");
  EXPECT_TRUE(recovered.is_valid());
  EXPECT_EQ("fallback.wav", recovered.title());
}

#else

TEST(MtpLoader, DisabledWithoutLibmtp) {
  EXPECT_TRUE(MtpLoader::LoadSongs("missing").empty());
}

#endif

TEST(MtpLoader, UrlHelpers) {
  EXPECT_EQ("ABC123", MtpLoader::ParseHost("mtp://ABC123/99"));
  EXPECT_EQ(99u, MtpLoader::ParseItemId("mtp://ABC123/99"));
  EXPECT_EQ("mtp://ABC123/99", MtpLoader::MakeUrl("ABC123", 99));
  EXPECT_TRUE(MtpLoader::ParseHost("file:///tmp/song.mp3").empty());
  EXPECT_EQ(0u, MtpLoader::ParseItemId("mtp://ABC123"));
  EXPECT_EQ(0u, MtpLoader::ParseItemId("http://example.com/1"));
}

TEST(DeviceDatabaseBackend, AddFindReplaceAndRemove) {
  const std::string path = "/tmp/strawberry-device-db-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  db.Open();
  ASSERT_TRUE(db.handle());

  DeviceDatabaseBackend backend(&db);
  ASSERT_TRUE(backend.Init());

  DeviceDatabaseBackend::Device device;
  device.unique_id = "mtp:ABC123";
  device.friendly_name = "Walkman";
  device.icon_name = "multimedia-player-symbolic";
  device.size = 16000000000LL;
  const int id = backend.AddDevice(device);
  ASSERT_GE(id, 0);

  const DeviceDatabaseBackend::Device found = backend.FindByUniqueId("mtp:ABC123");
  EXPECT_EQ(id, found.id);
  EXPECT_EQ("Walkman", found.friendly_name);
  EXPECT_EQ(DeviceDatabaseBackend::kDeviceSchemaVersion, found.schema_version);

  const int again = backend.AddDevice(device);
  EXPECT_EQ(id, again);
  EXPECT_EQ(1u, backend.GetAllDevices().size());

  Song song(Song::Source::Device);
  song.set_url("mtp://ABC123/42");
  song.set_title("Helplessness Blues");
  song.set_artist("Fleet Foxes");
  song.set_album("Helplessness Blues");
  song.set_genre("Folk");
  song.set_composer("Robin Pecknold");
  song.set_track(2);
  song.set_year(2011);
  song.set_length_nanosec(301000000000LL);
  song.set_bitrate(256);
  song.set_samplerate(44100);
  song.set_filesize(12345);
  song.set_basefilename("42");
  song.set_playcount(4);
  song.set_filetype(Song::FileType::MPEG);
  song.set_valid(true);
  ASSERT_TRUE(backend.ReplaceSongs(id, {song}));

  const SongList songs = backend.Songs(id);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Helplessness Blues", songs.front().title());
  EXPECT_EQ("Fleet Foxes", songs.front().artist());
  EXPECT_EQ("mtp://ABC123/42", songs.front().url());
  EXPECT_EQ(301000000000LL, songs.front().length_nanosec());
  EXPECT_EQ(4u, songs.front().playcount());
  EXPECT_EQ(Song::FileType::MPEG, songs.front().filetype());

  backend.RemoveDevice(id);
  EXPECT_TRUE(backend.GetAllDevices().empty());
  EXPECT_TRUE(backend.Songs(id).empty());
  db.Close();
  unlink(path.c_str());
}
