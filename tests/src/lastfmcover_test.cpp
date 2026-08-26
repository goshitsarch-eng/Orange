#include "covermanager/lastfmcoverprovider.h"

#include <gtest/gtest.h>

TEST(LastFmCoverProvider, SignsSortedParamsWithSecret) {
  const std::map<std::string, std::string> params = {
      {"album", "Dummy"}, {"api_key", LastFmCoverProvider::kApiKey}, {"method", "album.search"}};
  const std::string signature = LastFmCoverProvider::Sign(params);
  EXPECT_EQ(32u, signature.size());
  EXPECT_EQ(signature, LastFmCoverProvider::Sign(params));
  EXPECT_NE(signature, LastFmCoverProvider::Sign({{"album", "Other"}, {"api_key", LastFmCoverProvider::kApiKey}, {"method", "album.search"}}));
}

TEST(LastFmCoverProvider, FormBodyEscapesValues) {
  const std::string body = LastFmCoverProvider::FormBody({{"album", "Dummy Album"}, {"method", "album.search"}});
  EXPECT_NE(std::string::npos, body.find("album=Dummy%20Album"));
  EXPECT_NE(std::string::npos, body.find("method=album.search"));
}

TEST(LastFmCoverProvider, ImageSizeOrderAndUpgrade) {
  EXPECT_GT(LastFmCoverProvider::ImageSizeFromString("extralarge"), LastFmCoverProvider::ImageSizeFromString("small"));
  EXPECT_EQ("https://lastfm.example/i/740x0/cover.jpg", LastFmCoverProvider::UpgradeImageUrl("https://lastfm.example/i/300x300/cover.jpg"));
}

TEST(LastFmCoverProvider, ParsesAlbumSearchAndPicksLargest) {
  const std::string json = R"({
    "results": {
      "albummatches": {
        "album": [
          {
            "name": "Dummy",
            "artist": "Portishead",
            "image": [
              {"#text": "https://lastfm.example/i/300x300/small.jpg", "size": "small"},
              {"#text": "https://lastfm.example/i/300x300/xl.jpg", "size": "extralarge"}
            ]
          }
        ]
      }
    }
  })";
  const auto results = LastFmCoverProvider::ParseResults(json, "album");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Portishead", results.front().artist);
  EXPECT_EQ("Dummy", results.front().album);
  EXPECT_EQ("https://lastfm.example/i/740x0/xl.jpg", results.front().image_url);
}

TEST(LastFmCoverProvider, ParsesTrackSearchWithoutAlbumName) {
  const std::string json = R"({
    "results": {
      "trackmatches": {
        "track": [
          {
            "name": "Roads",
            "artist": "Portishead",
            "image": [{"#text": "https://lastfm.example/roads.jpg", "size": "large"}]
          }
        ]
      }
    }
  })";
  const auto results = LastFmCoverProvider::ParseResults(json, "track");
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("Portishead", results.front().artist);
  EXPECT_TRUE(results.front().album.empty());
  EXPECT_EQ("https://lastfm.example/roads.jpg", results.front().image_url);
}
