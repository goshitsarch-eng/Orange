#include "covermanager/deezercoverprovider.h"
#include "covermanager/discogscoverprovider.h"
#include "covermanager/musicbrainzcoverprovider.h"
#include "covermanager/musixmatchcoverprovider.h"
#include "covermanager/qobuzcoverprovider.h"
#include "covermanager/spotifycoverprovider.h"
#include "covermanager/tidalcoverprovider.h"
#include "utilities/musixmatchprovider.h"

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

TEST(MusixmatchProvider, StringFixup) {
  EXPECT_EQ("portishead", MusixmatchProvider::StringFixup("Portishead"));
  EXPECT_EQ("dummy", MusixmatchProvider::StringFixup("Dummy"));
  EXPECT_EQ("ac-dc", MusixmatchProvider::StringFixup("AC/DC"));
  EXPECT_EQ("dont-look-back", MusixmatchProvider::StringFixup("Don't Look Back"));
}

TEST(MusixmatchCoverProvider, ParsesNextDataCover) {
  const std::string html = R"(<html><script id="__NEXT_DATA__" type="application/json">{"props":{"pageProps":{"data":{"albumGet":{"data":{"artistName":"Portishead","name":"Dummy","coverImage500x500":"https://s.mxmcdn.net/500.jpg","coverImage800x800":"https://s.mxmcdn.net/800.jpg"}}}}}}</script></html>)";
  const auto results = MusixmatchCoverProvider::ParseAlbumPage(html, "Portishead", "Dummy");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("https://s.mxmcdn.net/800.jpg", results.front().image_url);
}

TEST(TidalCoverProvider, BuildsImageAndParsesItems) {
  EXPECT_EQ("https://resources.tidal.com/images/aa/bb/cc/1280x1280.jpg", TidalCoverProvider::ImageUrl("aa-bb-cc"));
  const std::string json = R"({
    "items": [
      {"artist":{"name":"Portishead"},"title":"Dummy - Disc 1","cover":"aa-bb-cc"}
    ]
  })";
  const auto results = TidalCoverProvider::ParseItems(json);
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Dummy", results.front().album);
  EXPECT_EQ("https://resources.tidal.com/images/aa/bb/cc/1280x1280.jpg", results.front().image_url);
}

TEST(SpotifyCoverProvider, ParsesAlbumImagesAtLeast300) {
  const std::string json = R"({
    "albums": {
      "items": [
        {
          "name": "Dummy",
          "artists": [{"name": "Portishead"}],
          "images": [
            {"url": "https://i.scdn.co/small.jpg", "width": 64, "height": 64},
            {"url": "https://i.scdn.co/large.jpg", "width": 640, "height": 640}
          ]
        }
      ]
    }
  })";
  const auto results = SpotifyCoverProvider::ParseResults(json, "albums");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("https://i.scdn.co/large.jpg", results.front().image_url);
  EXPECT_EQ("Portishead", results.front().artist);
}

TEST(QobuzCoverProvider, ParsesAlbumLargeImage) {
  const std::string json = R"json({
    "albums": {
      "items": [
        {
          "title": "Dummy (Remastered)",
          "artist": {"name": "Portishead"},
          "image": {"large": "https://static.qobuz.com/large.jpg", "small": "https://static.qobuz.com/small.jpg"}
        }
      ]
    }
  })json";
  const auto results = QobuzCoverProvider::ParseResults(json);
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Dummy", results.front().album);
  EXPECT_EQ("https://static.qobuz.com/large.jpg", results.front().image_url);
}
