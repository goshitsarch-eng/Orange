#include "subsonic/subsonicservice.h"

#include "core/settings.h"

#include <gtest/gtest.h>

TEST(SubsonicService, Md5AndHex) {
  EXPECT_EQ("5d41402abc4b2a76b9719d911017c592", SubsonicService::Md5Hex("hello"));
  EXPECT_EQ("70617373", SubsonicService::HexEncode("pass"));
}

TEST(SubsonicService, CreateUrlTokenAuth) {
  const std::string url = SubsonicService::CreateUrl("https://music.example.com", "alice", "secret", "search3", {{"query", "foxes"}}, false);
  EXPECT_NE(std::string::npos, url.find("https://music.example.com/rest/search3.view"));
  EXPECT_NE(std::string::npos, url.find("u=alice"));
  EXPECT_NE(std::string::npos, url.find("c=Orange"));
  EXPECT_NE(std::string::npos, url.find("v=1.11.0"));
  EXPECT_NE(std::string::npos, url.find("&s="));
  EXPECT_NE(std::string::npos, url.find("&t="));
  EXPECT_NE(std::string::npos, url.find("query=foxes"));
}

TEST(SubsonicService, CreateUrlHexAuth) {
  const std::string url = SubsonicService::CreateUrl("https://music.example.com/", "bob", "pw", "stream", {{"id", "12"}}, true);
  EXPECT_NE(std::string::npos, url.find("/rest/stream.view"));
  EXPECT_NE(std::string::npos, url.find("p=enc:" + SubsonicService::HexEncode("pw")));
  EXPECT_NE(std::string::npos, url.find("id=12"));
}

TEST(SubsonicService, LoadBuildsStreamUrl) {
  SubsonicService service(nullptr);
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetValue("url", "https://music.example.com");
  settings.SetValue("username", "alice");
  settings.SetValue("password", "secret");
  settings.Sync();
  service.ReloadSettings();
  EXPECT_TRUE(service.logged_in());
  const UrlHandler::LoadResult result = service.Load("subsonic://99");
  EXPECT_EQ(UrlHandler::LoadResult::Type::TrackAvailable, result.type);
  EXPECT_NE(std::string::npos, result.stream_url.find("/rest/stream.view"));
  EXPECT_NE(std::string::npos, result.stream_url.find("id=99"));
}
