#include "covermanager/deezercoverprovider.h"
#include "covermanager/discogscoverprovider.h"
#include "covermanager/musicbrainzcoverprovider.h"

#include <gtest/gtest.h>

TEST(MusicbrainzCoverProvider, BuildsQuotedSearchUrl) {
  const std::string url = MusicbrainzCoverProvider::SearchUrl("Portishead", "Dummy");
  EXPECT_NE(std::string::npos, url.find("https://musicbrainz.org/ws/2/release/"));
  EXPECT_NE(std::string::npos, url.find("fmt=json"));
  EXPECT_NE(std::string::npos, url.find("limit=8"));
  EXPECT_NE(std::string::npos, url.find("release"));
  EXPECT_NE(std::string::npos, url.find("artist"));
}

TEST(MusicbrainzCoverProvider, ParsesReleasesAndCoverArtArchive) {
  const std::string json = R"({
    "releases": [
      {
        "id": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
        "title": "Dummy",
        "artist-credit": [{"artist": {"name": "Someone Else"}}, {"artist": {"name": "Portishead"}}]
      }
    ]
  })";
  const auto results = MusicbrainzCoverProvider::ParseReleases(json, "Portishead");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Portishead", results.front().artist);
  EXPECT_EQ("Dummy", results.front().album);
  EXPECT_EQ("https://coverartarchive.org/release/aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee/front", results.front().image_url);
}

TEST(MusicbrainzCoverProvider, CollapsesUnmatchedMultiCreditToVarious) {
  const std::string json = R"({
    "releases": [
      {
        "id": "11111111-2222-3333-4444-555555555555",
        "title": "Compilation",
        "artist-credit": [{"artist": {"name": "A"}}, {"artist": {"name": "B"}}]
      }
    ]
  })";
  const auto results = MusicbrainzCoverProvider::ParseReleases(json, "Portishead");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Various artists", results.front().artist);
}

TEST(DeezerCoverProvider, SearchUrlUsesAlbumOrTrack) {
  EXPECT_NE(std::string::npos, DeezerCoverProvider::SearchUrl("Portishead", "Dummy", {}).find("/search/album"));
  EXPECT_NE(std::string::npos, DeezerCoverProvider::SearchUrl("Portishead", {}, "Roads").find("/search/track"));
}

TEST(DeezerCoverProvider, PrefersCoverXlAndStripsDisc) {
  const std::string json = R"({
    "data": [
      {
        "id": 1,
        "artist": {"name": "Portishead"},
        "type": "album",
        "title": "Dummy - Disc 2",
        "cover_big": "https://cdn.example/big.jpg",
        "cover_xl": "https://cdn.example/xl.jpg"
      }
    ]
  })";
  const auto results = DeezerCoverProvider::ParseResults(json);
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Portishead", results.front().artist);
  EXPECT_EQ("Dummy", results.front().album);
  EXPECT_EQ("https://cdn.example/xl.jpg", results.front().image_url);
}

TEST(DiscogsCoverProvider, SearchUrlIncludesKeysAndType) {
  const std::string url = DiscogsCoverProvider::SearchUrl("Portishead", "Dummy", "master");
  EXPECT_NE(std::string::npos, url.find("https://api.discogs.com/database/search"));
  EXPECT_NE(std::string::npos, url.find("type=master"));
  EXPECT_NE(std::string::npos, url.find("artist=portishead"));
  EXPECT_NE(std::string::npos, url.find("key="));
  EXPECT_NE(std::string::npos, url.find("secret="));
  EXPECT_FALSE(DiscogsCoverProvider::AccessKey().empty());
}

TEST(DiscogsCoverProvider, ParseSearchFiltersTitleSplit) {
  const std::string json = R"({
    "results": [
      {"id": 1, "title": "Portishead - Dummy", "resource_url": "https://api.discogs.com/masters/1"},
      {"id": 2, "title": "Other - Album", "resource_url": "https://api.discogs.com/masters/2"}
    ]
  })";
  const auto hits = DiscogsCoverProvider::ParseSearchResults(json, "Portishead", "Dummy");
  ASSERT_EQ(1u, hits.size());
  EXPECT_EQ("https://api.discogs.com/masters/1", hits.front().resource_url);
}

TEST(DiscogsCoverProvider, ParseReleasePrimaryImage) {
  const std::string json = R"({
    "artists": [{"name": "Portishead"}],
    "title": "Dummy",
    "images": [
      {"type": "secondary", "resource_url": "https://img.example/sec.jpg", "width": 1000, "height": 1000},
      {"type": "primary", "resource_url": "https://img.example/pri.jpg", "width": 600, "height": 600}
    ]
  })";
  const auto images = DiscogsCoverProvider::ParseReleaseImages(json, "Portishead", "Dummy");
  ASSERT_EQ(1u, images.size());
  EXPECT_EQ("https://img.example/pri.jpg", images.front().image_url);
}

TEST(DiscogsCoverProvider, RejectsSmallOrNonSquareImages) {
  EXPECT_FALSE(DiscogsCoverProvider::AcceptImage(200, 200));
  EXPECT_FALSE(DiscogsCoverProvider::AcceptImage(1000, 100));
  EXPECT_TRUE(DiscogsCoverProvider::AcceptImage(600, 600));
}
