#include "core/database.h"
#include "playlist/streamplaylistitem.h"
#include "playlistparsers/playlistparser.h"
#include "radios/radiobackend.h"
#include "radios/radiobrowsersearchmodel.h"
#include "radios/radiobrowserservice.h"
#include "radios/radiochannel.h"
#include "radios/radiomodel.h"
#include "radios/radioparadiseservice.h"
#include "radios/radiostreamplaylistitem.h"
#include "radios/somafmservice.h"

#include <gtest/gtest.h>
#include <unistd.h>

TEST(SomaFMService, ParsesPreferredQualityAndFormat) {
  const std::string json = R"({
    "channels": [
      {
        "title": "Groove Salad",
        "image": "https://somafm.com/img/groovesalad120.png",
        "playlists": [
          {"url": "https://somafm.com/groovesalad.pls", "format": "mp3", "quality": "highest"},
          {"url": "https://somafm.com/groovesalad64.pls", "format": "mp3", "quality": "low"}
        ]
      }
    ]
  })";
  const std::vector<RadioChannel> highest = SomaFMService::ParseChannels(json, "highest");
  ASSERT_EQ(1u, highest.size());
  EXPECT_EQ("Groove Salad MP3", highest.front().name);
  EXPECT_EQ("https://somafm.com/groovesalad.pls", highest.front().url);
  EXPECT_EQ("https://somafm.com/img/groovesalad120.png", highest.front().thumbnail_url);
  EXPECT_EQ(Song::Source::SomaFM, highest.front().source);

  const std::vector<RadioChannel> low = SomaFMService::ParseChannels(json, "64");
  ASSERT_EQ(1u, low.size());
  EXPECT_EQ("https://somafm.com/groovesalad64.pls", low.front().url);
}

TEST(SomaFMService, NormalizesLegacyBitrateQuality) {
  EXPECT_EQ("highest", SomaFMService::NormalizeQuality("128"));
  EXPECT_EQ("highest", SomaFMService::NormalizeQuality("256"));
  EXPECT_EQ("low", SomaFMService::NormalizeQuality("64"));
  EXPECT_EQ("highest", SomaFMService::NormalizeQuality({}));
}

TEST(RadioParadiseService, ParsesStreamsAndPrependsHttps) {
  const std::string json = R"({
    "channels": [
      {
        "chan_name": "Main Mix",
        "streams": [
          {"label": "320k AAC", "url": "stream.radioparadise.com/aac-320"},
          {"label": "128k AAC", "url": "https://stream.radioparadise.com/aac-128"}
        ]
      }
    ]
  })";
  const std::vector<RadioChannel> channels = RadioParadiseService::ParseChannels(json);
  ASSERT_EQ(2u, channels.size());
  EXPECT_EQ("Main Mix - 320k AAC", channels[0].name);
  EXPECT_EQ("https://stream.radioparadise.com/aac-320", channels[0].url);
  EXPECT_EQ("Main Mix - 128k AAC", channels[1].name);
  EXPECT_EQ("https://stream.radioparadise.com/aac-128", channels[1].url);
  EXPECT_EQ(Song::Source::RadioParadise, channels[0].source);
}

TEST(RadioParadiseService, EnsureAbsoluteUrl) {
  EXPECT_EQ("https://example.com/a", RadioParadiseService::EnsureAbsoluteUrl("example.com/a"));
  EXPECT_EQ("http://example.com/a", RadioParadiseService::EnsureAbsoluteUrl("http://example.com/a"));
  EXPECT_EQ("https://example.com/a", RadioParadiseService::EnsureAbsoluteUrl("https://example.com/a"));
}

TEST(RadioBrowserService, PrefersResolvedUrlAndSkipsNameless) {
  const std::string json = R"([
    {
      "name": " Station One ",
      "url": "https://example.com/stream",
      "url_resolved": "https://example.com/resolved",
      "favicon": "https://example.com/icon.png",
      "country": "Norway",
      "tags": "jazz",
      "codec": "MP3"
    },
    {"name": "", "url": "https://skip.example/stream"},
    {"name": "No URL"}
  ])";
  const std::vector<RadioChannel> channels = RadioBrowserService::ParseStations(json);
  ASSERT_EQ(1u, channels.size());
  EXPECT_EQ("Station One", channels.front().name);
  EXPECT_EQ("https://example.com/resolved", channels.front().url);
  EXPECT_EQ("https://example.com/icon.png", channels.front().thumbnail_url);
  EXPECT_EQ("Norway", channels.front().country);
  EXPECT_EQ("jazz", channels.front().tags);
  EXPECT_EQ("MP3", channels.front().codec);
  EXPECT_EQ(Song::Source::RadioBrowser, channels.front().source);
}

TEST(RadioBrowserService, SearchUrlUsesVotesAndHideBroken) {
  const std::string url = RadioBrowserService::SearchUrl("https://de1.api.radio-browser.info/", "groove salad", "NO", true, 25, 10);
  EXPECT_NE(std::string::npos, url.find("https://de1.api.radio-browser.info/json/stations/search"));
  EXPECT_NE(std::string::npos, url.find("name=groove%20salad"));
  EXPECT_NE(std::string::npos, url.find("countrycode=NO"));
  EXPECT_NE(std::string::npos, url.find("hidebroken=true"));
  EXPECT_NE(std::string::npos, url.find("limit=25"));
  EXPECT_NE(std::string::npos, url.find("offset=10"));
  EXPECT_NE(std::string::npos, url.find("order=votes"));
  EXPECT_NE(std::string::npos, url.find("reverse=true"));
}

TEST(RadioServices, PlaylistParserResolvesSomaFMPls) {
  PlaylistParser parser;
  const std::string pls = "[playlist]\nNumberOfEntries=1\nFile1=https://ice1.somafm.com/groovesalad-128-mp3\n";
  const SongList songs = parser.LoadFromData(pls, "https://somafm.com/groovesalad.pls");
  ASSERT_FALSE(songs.empty());
  EXPECT_EQ("https://ice1.somafm.com/groovesalad-128-mp3", songs.front().url());
}

TEST(RadioChannel, ToSongAndPlaylistItem) {
  RadioChannel channel;
  channel.name = "Groove Salad";
  channel.url = "https://ice1.somafm.com/groovesalad-128-mp3";
  channel.thumbnail_url = "https://somafm.com/img/groovesalad120.png";
  channel.tags = "ambient";
  channel.source = Song::Source::SomaFM;
  const Song song = channel.ToSong();
  EXPECT_EQ("Groove Salad", song.title());
  EXPECT_EQ(channel.url, song.url());
  EXPECT_EQ(channel.thumbnail_url, song.art_automatic());
  EXPECT_EQ("ambient", song.genre());
  EXPECT_EQ(Song::Source::SomaFM, song.source());
  EXPECT_TRUE(song.is_valid());

  RadioStreamPlaylistItem item(channel);
  EXPECT_EQ(channel.url, item.EffectiveMetadata().url());
  EXPECT_EQ("Groove Salad", item.EffectiveMetadata().title());
}

TEST(RadioModel, LabelsAndSearchResults) {
  RadioChannel channel;
  channel.name = "Groove Salad";
  channel.country = "US";
  channel.codec = "mp3";
  RadioModel model;
  model.SetChannels({channel});
  EXPECT_EQ(1, model.row_count());
  EXPECT_EQ("Groove Salad · US (mp3)", model.Label(channel));
  RadioChannel other;
  other.name = "Search hit";
  model.SetSearchResults({other});
  EXPECT_EQ(1, model.row_count());
  EXPECT_EQ("Search hit", model.visible().front().name);

  RadioBrowserSearchModel search;
  search.SetResults({channel, other});
  EXPECT_EQ(2, search.row_count());
}

TEST(RadioBackend, PersistAndRemoveSource) {
  const std::string path = "/tmp/strawberry-radio-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  RadioBackend backend(&db);
  RadioChannel channel;
  channel.name = "Custom";
  channel.url = "https://example.com/stream.mp3";
  channel.thumbnail_url = "https://example.com/art.png";
  channel.source = Song::Source::Stream;
  backend.Save(channel);
  const std::vector<RadioChannel> loaded = backend.Load();
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("Custom", loaded.front().name);
  EXPECT_EQ(channel.url, loaded.front().url);
  EXPECT_EQ(Song::Source::Stream, loaded.front().source);
  backend.RemoveSource(Song::Source::Stream);
  EXPECT_TRUE(backend.Load().empty());
  unlink(path.c_str());
}
