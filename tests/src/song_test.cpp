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

TEST(Song, ExoticFiletypesMatchQt) {
  EXPECT_EQ(Song::FileType::OggSpeex, Song::FiletypeByExtension("spx"));
  EXPECT_EQ(Song::FileType::OggSpeex, Song::FiletypeByExtension("speex"));
  EXPECT_EQ(Song::FileType::OggSpeex, Song::FiletypeByMimeType("audio/x-speex"));
  EXPECT_EQ(Song::FileType::TrueAudio, Song::FiletypeByExtension("tta"));
  EXPECT_EQ(Song::FileType::DSF, Song::FiletypeByExtension("dsf"));
  EXPECT_EQ(Song::FileType::DSDIFF, Song::FiletypeByExtension("dsd"));
  EXPECT_EQ(Song::FileType::DSDIFF, Song::FiletypeByExtension("dff"));
  EXPECT_EQ(Song::FileType::MOD, Song::FiletypeByExtension("mod"));
  EXPECT_EQ(Song::FileType::MOD, Song::FiletypeByExtension("module"));
  EXPECT_EQ(Song::FileType::S3M, Song::FiletypeByExtension("s3m"));
  EXPECT_EQ(Song::FileType::XM, Song::FiletypeByExtension("xm"));
  EXPECT_EQ(Song::FileType::IT, Song::FiletypeByExtension("it"));
  EXPECT_EQ(Song::FileType::WAV, Song::FiletypeByExtension("wave"));
  EXPECT_EQ(Song::FileType::WavPack, Song::FiletypeByExtension("wavpack"));
  EXPECT_EQ(Song::FileType::MPC, Song::FiletypeByExtension("mpp"));
  EXPECT_EQ(Song::FileType::AIFF, Song::FiletypeByExtension("aifc"));
  EXPECT_TRUE(Song::IsAudioFile("chip.mod"));
  EXPECT_TRUE(Song::IsAudioFile("track.spx"));
  EXPECT_TRUE(Song::IsAudioFile("album.dff"));
  EXPECT_FALSE(Song::IsAudioFile("notes.lrc"));
  EXPECT_FALSE(Song::IsAudioFile("temp.wvc"));
  EXPECT_EQ("Speex", Song::FiletypeToString(Song::FileType::OggSpeex));
  EXPECT_EQ("DSF", Song::FiletypeToString(Song::FileType::DSF));
  EXPECT_EQ("DSDIFF", Song::FiletypeToString(Song::FileType::DSDIFF));
  EXPECT_EQ("Module Music Format", Song::FiletypeToString(Song::FileType::MOD));
  EXPECT_EQ("SNES SPC700", Song::FiletypeToString(Song::FileType::SPC));
  EXPECT_EQ("VGM", Song::FiletypeToString(Song::FileType::VGM));
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

TEST(Song, RadioAndMetadataMatchQt) {
  Song radio;
  radio.set_source(Song::Source::SomaFM);
  radio.set_url("http://somafm.example/groove");
  radio.set_artist("DJ");
  radio.set_title("Mix");
  EXPECT_TRUE(radio.is_radio());
  EXPECT_FALSE(radio.is_stream_service());
  EXPECT_TRUE(radio.is_stream());
  EXPECT_TRUE(radio.is_metadata_good());
  Song tidal;
  tidal.set_source(Song::Source::Tidal);
  tidal.set_url("tidal://track/1");
  tidal.set_artist("A");
  tidal.set_title("B");
  EXPECT_FALSE(tidal.is_radio());
  EXPECT_TRUE(tidal.is_stream_service());
  EXPECT_TRUE(tidal.is_stream());
  Song incomplete;
  incomplete.set_source(Song::Source::Stream);
  incomplete.set_url("http://x");
  EXPECT_FALSE(incomplete.is_metadata_good());
}

TEST(Song, IsEditableRequiresLocalWritableFile) {
  Song local;
  local.set_valid(true);
  local.set_source(Song::Source::LocalFile);
  local.set_url("file:///tmp/song.flac");
  EXPECT_TRUE(local.IsEditable());

  Song collection;
  collection.set_valid(true);
  collection.set_source(Song::Source::Collection);
  collection.set_url("file:///music/album/track.flac");
  EXPECT_TRUE(collection.IsEditable());

  Song stream;
  stream.set_valid(true);
  stream.set_source(Song::Source::Stream);
  stream.set_url("http://example.invalid/live");
  EXPECT_FALSE(stream.IsEditable());

  Song cdda;
  cdda.set_valid(true);
  cdda.set_source(Song::Source::CDDA);
  EXPECT_FALSE(cdda.IsEditable());

  Song cue;
  cue.set_valid(true);
  cue.set_source(Song::Source::LocalFile);
  cue.set_url("file:///tmp/album.flac");
  cue.set_cue_path("/tmp/album.cue");
  EXPECT_FALSE(cue.IsEditable());

  Song invalid;
  invalid.set_source(Song::Source::LocalFile);
  EXPECT_FALSE(invalid.IsEditable());
}
