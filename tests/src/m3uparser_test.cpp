#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "playlistparsers/asxiniparser.h"
#include "playlistparsers/asxparser.h"
#include "playlistparsers/cueparser.h"
#include "playlistparsers/m3uparser.h"
#include "playlistparsers/parserbase.h"
#include "playlistparsers/playlistparser.h"
#include "playlistparsers/plsparser.h"
#include "playlistparsers/wplparser.h"
#include "playlistparsers/xspfparser.h"
#include "utilities/fileutils.h"

#include <glib.h>
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

TEST(ParserBase, URLOrFilenameHonorsPathType) {
  ParserBase::SetPathTypeOverride(static_cast<int>(PlaylistSettings::PathType::Absolute));
  EXPECT_EQ("/music/album/track.flac", ParserBase::URLOrFilename("file:///music/album/track.flac", "/music/album"));
  ParserBase::SetPathTypeOverride(static_cast<int>(PlaylistSettings::PathType::Relative));
  EXPECT_EQ("track.flac", ParserBase::URLOrFilename("file:///music/album/track.flac", "/music/album"));
  ParserBase::SetPathTypeOverride(static_cast<int>(PlaylistSettings::PathType::Automatic));
  EXPECT_EQ("track.flac", ParserBase::URLOrFilename("file:///music/album/track.flac", "/music/album"));
  EXPECT_EQ("/other/track.flac", ParserBase::URLOrFilename("file:///other/track.flac", "/music/album"));
  ParserBase::SetPathTypeOverride(-1);
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

TEST(PlaylistParser, EnrichCueFromAudioFile) {
  SongList songs;
  Song track;
  track.set_title("One");
  track.set_beginning_nanosec(1000000000LL);
  track.set_valid(true);
  songs.push_back(track);
  Song file;
  file.set_bitrate(1411);
  file.set_samplerate(44100);
  file.set_bitdepth(16);
  file.set_filesize(12345);
  file.set_year(2008);
  file.set_genre("Folk");
  file.set_albumartist("Fleet Foxes");
  file.set_length_nanosec(5000000000LL);
  file.set_art_embedded(true);
  PlaylistParser::EnrichFromAudioFile(&songs, file);
  EXPECT_EQ(1411, songs[0].bitrate());
  EXPECT_EQ(44100, songs[0].samplerate());
  EXPECT_EQ(16, songs[0].bitdepth());
  EXPECT_EQ(12345, songs[0].filesize());
  EXPECT_EQ(2008, songs[0].year());
  EXPECT_EQ("Folk", songs[0].genre());
  EXPECT_EQ("Fleet Foxes", songs[0].albumartist());
  EXPECT_TRUE(songs[0].art_embedded());
  EXPECT_EQ(4000000000LL, songs[0].length_nanosec());
}

TEST(M3UParser, ParsesExtinfArtistTitleAndLength) {
  M3UParser::Metadata meta;
  ASSERT_TRUE(M3UParser::ParseMetadata("#EXTINF:123,Portishead - Roads", &meta));
  EXPECT_EQ("Portishead", meta.artist);
  EXPECT_EQ("Roads", meta.title);
  EXPECT_EQ(123000000000LL, meta.length_nanosec);
  ASSERT_TRUE(M3UParser::ParseMetadata("#EXTINF:12,Just a title", &meta));
  EXPECT_TRUE(meta.artist.empty());
  EXPECT_EQ("Just a title", meta.title);
  EXPECT_FALSE(M3UParser::IsNestedPlaylistReference("http://example/list.m3u8"));
  EXPECT_TRUE(M3UParser::IsNestedPlaylistReference("other.m3u"));
}

TEST(M3UParser, ExpandsNestedPlaylistAndStopsCycles) {
  const std::string dir = "/tmp/strawberry-m3u-" + std::to_string(getpid());
  g_mkdir_with_parents(dir.c_str(), 0755);
  const std::string child = dir + "/child.m3u";
  const std::string parent = dir + "/parent.m3u";
  ASSERT_TRUE(FileUtils::WriteFile(child, "http://example/a.mp3\nparent.m3u\n"));
  ASSERT_TRUE(FileUtils::WriteFile(parent, "#EXTM3U\n#EXTINF:10,Artist - Title\nchild.m3u\n"));
  const SongList songs = M3UParser().Load(FileUtils::ReadFile(parent), parent);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("http://example/a.mp3", songs[0].url());
  unlink(child.c_str());
  unlink(parent.c_str());
  rmdir(dir.c_str());
}

TEST(PLSParser, LoadsIndexedTitleAndLength) {
  const std::string data = "[playlist]\nFile1=http://example/a.mp3\nTitle1=Roads\nLength1=75\nFile2=/tmp/b.flac\nTitle2=Glory\n";
  const SongList songs = PLSParser().Load(data, "/tmp/list.pls");
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ("Roads", songs[0].title());
  EXPECT_EQ(75000000000LL, songs[0].length_nanosec());
  EXPECT_EQ("Glory", songs[1].title());
}

TEST(XSPFParser, ParsesTrackMetadataAndDurationMs) {
  const std::string data = R"xml(<?xml version="1.0"?>
<playlist version="1" xmlns="http://xspf.org/ns/0/">
  <trackList>
    <track>
      <location>http://example/roads.mp3</location>
      <title>Roads</title>
      <creator>Portishead</creator>
      <album>Dummy</album>
      <duration>303000</duration>
      <trackNum>8</trackNum>
      <image>http://example/cover.jpg</image>
    </track>
  </trackList>
</playlist>)xml";
  const SongList songs = XSPFParser().Load(data);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Roads", songs[0].title());
  EXPECT_EQ("Portishead", songs[0].artist());
  EXPECT_EQ("Dummy", songs[0].album());
  EXPECT_EQ(8, songs[0].track());
  EXPECT_EQ(303000000000LL, songs[0].length_nanosec());
  EXPECT_EQ("http://example/cover.jpg", songs[0].art_manual());
}

TEST(ASXParser, ParsesEntryRefTitleAndAuthor) {
  const std::string data = R"xml(<asx version="3.0">
  <entry>
    <title>Groove Salad</title>
    <author>SomaFM</author>
    <ref href="http://ice.somafm.com/groovesalad"/>
  </entry>
</asx>)xml";
  const SongList songs = ASXParser().Load(data);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Groove Salad", songs[0].title());
  EXPECT_EQ("SomaFM", songs[0].artist());
  EXPECT_EQ("http://ice.somafm.com/groovesalad", songs[0].url());
}

TEST(AsxIniParser, LoadsReferenceEntries) {
  const SongList songs = AsxIniParser().Load("[Reference]\nRef1=http://example/a.mp3\nRef2=http://example/b.mp3\n");
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ("http://example/a.mp3", songs[0].url());
}

TEST(WplParser, ParsesMediaSrcAndSavesSmil) {
  const SongList songs = WplParser().Load("<smil><body><seq><media src=\"http://example/a.mp3\"/></seq></body></smil>");
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("http://example/a.mp3", songs[0].url());
  const std::string path = "/tmp/strawberry-test-" + std::to_string(getpid()) + ".wpl";
  ASSERT_TRUE(WplParser().Save(path, songs));
  const std::string saved = FileUtils::ReadFile(path);
  EXPECT_NE(std::string::npos, saved.find("<smil>"));
  EXPECT_NE(std::string::npos, saved.find("ItemCount"));
  unlink(path.c_str());
}

TEST(CueParser, ReadsRemGenreDateDiscAndComposer) {
  const std::string cue = R"(PERFORMER "Album Artist"
TITLE "Album"
REM GENRE "Trip-Hop"
REM DATE 1994
REM DISC 1
SONGWRITER "Geoff Barrow"
FILE "album.flac" WAVE
  TRACK 01 AUDIO
    TITLE "One"
    PERFORMER "Artist"
    INDEX 01 00:00:00
)";
  const SongList songs = CueParser().Load(cue, "/tmp/album.cue");
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Trip-Hop", songs[0].genre());
  EXPECT_EQ(1994, songs[0].year());
  EXPECT_EQ(1, songs[0].disc());
  EXPECT_EQ("Geoff Barrow", songs[0].composer());
}

TEST(PlaylistParser, PicksParserByExtensionAndMagic) {
  PlaylistParser parser;
  EXPECT_EQ("M3U", parser.ParserForExtension("m3u8")->name());
  EXPECT_EQ("PLS", parser.ParserForMagic("[playlist]\nFile1=x")->name());
  EXPECT_EQ("XSPF", parser.ParserForMagic("<playlist><trackList></trackList>")->name());
  EXPECT_EQ("ASX/INI", parser.ParserForMagic("[Reference]\nRef1=x")->name());
}
