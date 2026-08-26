#include "discord/discord.h"
#include "discord/discordart.h"
#include "core/song.h"

#include <gtest/gtest.h>

TEST(DiscordRichPresence, HandshakeAndClearJson) {
  EXPECT_EQ(R"({"v":1,"client_id":"1352351827206733974"})", DiscordRichPresence::HandshakeJson(DiscordRichPresence::kApplicationId));
  EXPECT_EQ(R"({"cmd":"SET_ACTIVITY","nonce":"3","args":{"pid":9,"activity":null}})", DiscordRichPresence::ClearActivityJson(9, 3));
  EXPECT_NE(std::string::npos, DiscordRichPresence::SocketPath(0).find("discord-ipc-0"));
}

TEST(DiscordRichPresence, SetActivityJsonIncludesSongAndTimestamps) {
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_length_nanosec(300000000000LL);
  song.set_valid(true);
  const std::string json = DiscordRichPresence::SetActivityJson(song, true, 1, 1000, 42, 7);
  EXPECT_NE(std::string::npos, json.find("\"cmd\":\"SET_ACTIVITY\""));
  EXPECT_NE(std::string::npos, json.find("\"details\":\"Roads\""));
  EXPECT_NE(std::string::npos, json.find("\"state\":\"Portishead\""));
  EXPECT_NE(std::string::npos, json.find("\"large_text\":\"on Dummy\""));
  EXPECT_NE(std::string::npos, json.find("\"status_display_type\":1"));
  EXPECT_NE(std::string::npos, json.find("\"start\":1000"));
  EXPECT_NE(std::string::npos, json.find("\"end\":1300"));
  EXPECT_NE(std::string::npos, json.find("\"pid\":42"));
}

TEST(DiscordRichPresence, JsonEscapeAndShortArtist) {
  EXPECT_EQ("say \\\"hi\\\"", DiscordRichPresence::JsonEscape("say \"hi\""));
  Song song;
  song.set_title("X");
  song.set_artist("A");
  const std::string json = DiscordRichPresence::SetActivityJson(song, false, 0, 0, 1, 1);
  EXPECT_NE(std::string::npos, json.find("\"state\":\"A \""));
  EXPECT_EQ(std::string::npos, json.find("timestamps"));
}

TEST(DiscordArt, UsesHttpCoverOrEmbedded) {
  EXPECT_EQ("embedded_cover", DiscordArt::ArtKey({}));
  EXPECT_EQ("embedded_cover", DiscordArt::ArtKey("file:///covers/a.jpg"));
  EXPECT_EQ("https://example.com/a.jpg", DiscordArt::ArtKey("https://example.com/a.jpg"));
  EXPECT_EQ("https://cdn.example/a.jpg", DiscordArt::SongArtUrl("https://cdn.example/a.jpg", "file:///local.jpg"));
}

TEST(DiscordRichPresence, DisabledDoesNotConnect) {
  DiscordRichPresence discord;
  EXPECT_FALSE(discord.enabled());
  Song song;
  song.set_title("Roads");
  discord.UpdatePresence(song, true);
  EXPECT_FALSE(discord.connected());
}
