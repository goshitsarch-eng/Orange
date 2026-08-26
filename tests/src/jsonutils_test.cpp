#include "utilities/jsonutils.h"

#include <gtest/gtest.h>

TEST(JsonUtils, LastFmCoverUrl) {
  const std::string json = R"({
    "album": {
      "image": [
        {"#text": "https://lastfm.example/small.jpg", "size": "small"},
        {"#text": "https://lastfm.example/extralarge.jpg", "size": "extralarge"}
      ]
    }
  })";
  EXPECT_EQ("https://lastfm.example/extralarge.jpg", JsonUtils::FindCoverUrl(json));
}

TEST(JsonUtils, DeezerCoverUrl) {
  const std::string json = R"({"data":[{"cover_xl":"https://cdn.example/cover-xl.jpg","title":"Album"}]})";
  EXPECT_EQ("https://cdn.example/cover-xl.jpg", JsonUtils::FindCoverUrl(json));
}

TEST(JsonUtils, MusicBrainzCoverArtArchive) {
  const std::string json = R"({"releases":[{"id":"aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee","title":"Album"}]})";
  EXPECT_EQ("https://coverartarchive.org/release/aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee/front", JsonUtils::FindCoverUrl(json));
}

TEST(JsonUtils, LyricsOvh) {
  const std::string json = R"({"lyrics":"Hello\nWorld"})";
  EXPECT_EQ("Hello\nWorld", JsonUtils::ExtractLyrics(json));
}

TEST(JsonUtils, LrcLibPlainLyrics) {
  const std::string json = R"({"plainLyrics":"Verse one","syncedLyrics":"[00:01.00] Verse one"})";
  EXPECT_EQ("Verse one", JsonUtils::ExtractLyrics(json));
}

TEST(JsonUtils, HtmlLyrics) {
  const std::string html = "<html><body><div class=\"lyrics\">Line one<br>Line two</div></body></html>";
  const std::string lyrics = JsonUtils::ExtractLyrics(html);
  EXPECT_NE(std::string::npos, lyrics.find("Line one"));
  EXPECT_NE(std::string::npos, lyrics.find("Line two"));
}

TEST(JsonUtils, ParseStreamingSongs) {
  const std::string json = R"({"data":[{"title":"Song A","artist":{"name":"Artist A"},"album":{"title":"Album A"},"preview":"https://cdn.example/a.mp3"}]})";
  const SongList songs = JsonUtils::ParseSongs(json, Song::Source::Stream);
  ASSERT_FALSE(songs.empty());
  EXPECT_EQ("Song A", songs.front().title());
  EXPECT_EQ("Artist A", songs.front().artist());
  EXPECT_EQ("Album A", songs.front().album());
  EXPECT_EQ("https://cdn.example/a.mp3", songs.front().url());
}

TEST(JsonUtils, ImageMagic) {
  const std::string jpeg("\xFF\xD8\xFF\xE0xxxx", 8);
  const std::string png("\x89PNG\r\n\x1A\n", 8);
  EXPECT_TRUE(JsonUtils::LooksLikeImage(jpeg));
  EXPECT_TRUE(JsonUtils::LooksLikeImage(png));
  EXPECT_FALSE(JsonUtils::LooksLikeImage("not an image"));
}

TEST(JsonUtils, GetStringPath) {
  const std::string json = R"({"album":{"name":"Dummy"}})";
  EXPECT_EQ("Dummy", JsonUtils::GetString(json, {"album", "name"}));
}

TEST(JsonUtils, SubsonicSearchSongs) {
  const std::string json = R"({
    "subsonic-response": {
      "searchResult3": {
        "song": [
          {"id": "42", "title": "Ragged Wood", "artist": "Fleet Foxes", "album": "Fleet Foxes", "duration": 301, "bitRate": 320, "track": 2, "year": 2008}
        ]
      }
    }
  })";
  const SongList songs = JsonUtils::ParseSubsonicSongs(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Ragged Wood", songs.front().title());
  EXPECT_EQ("Fleet Foxes", songs.front().artist());
  EXPECT_EQ("subsonic://42", songs.front().url());
  EXPECT_EQ(Song::Source::Subsonic, songs.front().source());
}

TEST(JsonUtils, TidalTracks) {
  const std::string json = R"({"items":[{"id":99,"title":"Song","artist":{"name":"Artist"},"album":{"title":"Album","cover":"aa-bb"},"duration":180}]})";
  const SongList songs = JsonUtils::ParseTidalTracks(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Song", songs.front().title());
  EXPECT_EQ("tidal://99", songs.front().url());
}

TEST(JsonUtils, SpotifyTracks) {
  const std::string json = R"({"tracks":{"items":[{"id":"abc","name":"Song","artists":[{"name":"Artist"}],"album":{"name":"Album"},"preview_url":"https://p.example/a.mp3","duration_ms":123000}]}})";
  const SongList songs = JsonUtils::ParseSpotifyTracks(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Song", songs.front().title());
  EXPECT_EQ("spotify://abc", songs.front().url());
  EXPECT_EQ("https://p.example/a.mp3", songs.front().stream_url());
}

TEST(JsonUtils, QobuzTracks) {
  const std::string json = R"({"tracks":{"items":[{"id":7,"title":"Song","performer":{"name":"Artist"},"album":{"title":"Album"},"duration":200}]}})";
  const SongList songs = JsonUtils::ParseQobuzTracks(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Song", songs.front().title());
  EXPECT_EQ("qobuz://7", songs.front().url());
}

TEST(JsonUtils, MusicBrainzRecordings) {
  const std::string json = R"({
    "recordings": [
      {
        "id": "rec-1",
        "title": "Helplessness Blues",
        "artist-credit": [{"name": "Fleet Foxes", "artist": {"id": "art-1", "name": "Fleet Foxes"}}],
        "releases": [{"id": "rel-1", "title": "Helplessness Blues", "date": "2011-05-03"}]
      }
    ]
  })";
  const SongList songs = JsonUtils::ParseMusicBrainzRecordings(json);
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Helplessness Blues", songs.front().title());
  EXPECT_EQ("Fleet Foxes", songs.front().artist());
  EXPECT_EQ("Helplessness Blues", songs.front().album());
  EXPECT_EQ(2011, songs.front().year());
  EXPECT_EQ("rec-1", songs.front().musicbrainz_recording_id());
}
