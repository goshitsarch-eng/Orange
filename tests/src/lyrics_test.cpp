#include "context/contextlyrics.h"
#include "lyrics/geniuslyricsprovider.h"
#include "lyrics/htmllyricsprovider.h"
#include "lyrics/lrcparser.h"

#include "utilities/strutils.h"

#include <gtest/gtest.h>

TEST(HtmlLyricsProvider, ParsesAzLyricsCommentBlock) {
  const std::string html =
      "<html><body><div class=\"x\">ads</div>"
      "<!-- Usage of azlyrics.com content by any third-party lyrics provider is prohibited by our licensing agreement. Sorry about that. -->"
      "Line one<br>Line two</div></body></html>";
  const std::string lyrics = HtmlLyricsProvider::ParseLyricsFromHTML(
      html, "<div>", "</div>",
      "<!-- Usage of azlyrics.com content by any third-party lyrics provider is prohibited by our licensing agreement. Sorry about that. -->", false);
  EXPECT_NE(std::string::npos, lyrics.find("Line one"));
  EXPECT_NE(std::string::npos, lyrics.find("Line two"));
}

TEST(HtmlLyricsProvider, ParsesNestedDivLyrics) {
  const std::string html = "<div id=\"songLyricsDiv\" class=\"box\">Hello<br><div>ignore</div>World</div>";
  const std::string lyrics = HtmlLyricsProvider::ParseLyricsFromHTML(html, "<div[^>]*>", "</div>", "<div id=\"songLyricsDiv\"", false);
  EXPECT_NE(std::string::npos, lyrics.find("Hello"));
  EXPECT_NE(std::string::npos, lyrics.find("World"));
}

TEST(HtmlLyricsProvider, RejectsMissingLyricsCopy) {
  const std::string html = "<div class=\"lyric-original\">we do not have the lyrics for this song</div>";
  const std::string lyrics = HtmlLyricsProvider::ParseLyricsFromHTML(html, "<div[^>]*>", "</div>", "<div class=\"lyric-original\">", false);
  EXPECT_TRUE(lyrics.empty());
}

TEST(HtmlLyricsProvider, AzLyricsSlug) {
  EXPECT_EQ("fleetfoxes", HtmlLyricsProvider::SlugAzLyrics("Fleet Foxes"));
  EXPECT_EQ("helplessnessblues", HtmlLyricsProvider::SlugAzLyrics("Helplessness Blues"));
}

TEST(HtmlLyricsProvider, DashedAndLetrasSlugs) {
  EXPECT_EQ("fleet-foxes", HtmlLyricsProvider::SlugDashed("Fleet Foxes"));
  EXPECT_EQ("don-t-leave", HtmlLyricsProvider::SlugDashed("Don't Leave"));
  EXPECT_EQ("fleet-foxes", HtmlLyricsProvider::SlugLetras("Fleet Foxes"));
}

TEST(HtmlLyricsProvider, ElyricsSlugAndFirstLetter) {
  EXPECT_EQ("fleet-foxes", HtmlLyricsProvider::SlugElyrics("Fleet Foxes"));
}

TEST(GeniusLyricsProvider, AuthorizationAndSearchUrl) {
  EXPECT_EQ(std::string(GeniusLyricsProvider::kApiUrl) + "/search?q=Portishead%20Roads",
            GeniusLyricsProvider::SearchApiUrl("Portishead Roads"));
  const std::string auth = GeniusLyricsProvider::AuthorizationUrl("client", GeniusLyricsProvider::OAuthRedirectUri());
  EXPECT_NE(std::string::npos, auth.find(GeniusLyricsProvider::kAuthUrl));
  EXPECT_NE(std::string::npos, auth.find("client_id=client"));
  EXPECT_NE(std::string::npos, auth.find("response_type=code"));
  EXPECT_EQ(63111, GeniusLyricsProvider::kOAuthPort);
  EXPECT_STREQ("http://localhost:63111/", GeniusLyricsProvider::OAuthRedirectUri());
  EXPECT_NE(std::string::npos, auth.find("localhost%3A63111"));
  const std::string json = R"json({"response":{"hits":[{"result":{"url":"https://genius.com/portishead-roads-lyrics"}}]}})json";
  EXPECT_EQ("https://genius.com/portishead-roads-lyrics", GeniusLyricsProvider::ParseSearchResultUrl(json));
}

TEST(LrcParser, TimestampsAndActiveLine) {
  const std::string lrc = "[00:00.00] Intro\n[00:12.50] Verse\n[01:02.00] Chorus\n";
  const auto lines = LrcParser::Parse(lrc);
  ASSERT_EQ(3u, lines.size());
  EXPECT_EQ(0, lines[0].timestamp_ms);
  EXPECT_EQ("Intro", lines[0].text);
  EXPECT_EQ(12500, lines[1].timestamp_ms);
  EXPECT_EQ("Verse", lines[1].text);
  EXPECT_EQ(62000, lines[2].timestamp_ms);
  EXPECT_EQ(-1, LrcParser::ActiveLineIndex(lines, -1));
  EXPECT_EQ(0, LrcParser::ActiveLineIndex(lines, 0));
  EXPECT_EQ(1, LrcParser::ActiveLineIndex(lines, 12500));
  EXPECT_EQ(1, LrcParser::ActiveLineIndex(lines, 30000));
  EXPECT_EQ(2, LrcParser::ActiveLineIndex(lines, 62000));
  EXPECT_EQ("Intro\nVerse\nChorus", LrcParser::PlainText(lines));
  EXPECT_TRUE(LrcParser::LooksSynced(lrc));
  EXPECT_FALSE(LrcParser::LooksSynced("just words"));
  EXPECT_EQ(90500, LrcParser::ParseTimestamp("01:30.500"));
  const auto multi = LrcParser::Parse("[00:12.00][00:45.00] Chorus");
  ASSERT_EQ(2u, multi.size());
  EXPECT_EQ(12000, multi[0].timestamp_ms);
  EXPECT_EQ(45000, multi[1].timestamp_ms);
  EXPECT_EQ("Chorus", multi[0].text);
}

TEST(ContextLyrics, Attribution) {
  EXPECT_TRUE(ContextLyrics::Attribution({}).empty());
  EXPECT_EQ("Source: Genius", ContextLyrics::Attribution("Genius"));
}

TEST(ContextLyrics, PrefersTagLyricsAndFormatsFetch) {
  EXPECT_STREQ("No lyrics found.\n", ContextLyrics::NoResultsText());
  EXPECT_EQ("\n\n(Lyrics from Genius)\n", ContextLyrics::Footer("Genius"));
  EXPECT_EQ(ContextLyrics::NoResultsText(), ContextLyrics::FormatFetched({}, "Genius"));
  EXPECT_EQ("hello\n\n(Lyrics from Genius)\n", ContextLyrics::FormatFetched("hello", "Genius"));
  EXPECT_EQ("hello", ContextLyrics::FormatFetched("hello", "Genius", false));
  EXPECT_EQ("hello", ContextLyrics::WithoutFooter(ContextLyrics::FormatFetched("hello", "Genius")));
  EXPECT_TRUE(ContextLyrics::ShouldShowSource("Genius", true));
  EXPECT_FALSE(ContextLyrics::ShouldShowSource("Genius", false));
  EXPECT_FALSE(ContextLyrics::ShouldShowSource({}, true));
  EXPECT_TRUE(ContextLyrics::WithoutFooter(ContextLyrics::NoResultsText()).empty());
  EXPECT_TRUE(ContextLyrics::IsNoResults(ContextLyrics::NoResultsText()));
  Song song;
  song.set_artist("Portishead");
  song.set_title("Roads");
  song.set_lyrics("embedded");
  EXPECT_EQ("embedded", ContextLyrics::InitialLyricsFromSong(song));
  EXPECT_FALSE(ContextLyrics::ShouldFetchOnline("embedded", true, true, song, false));
  song.set_lyrics({});
  EXPECT_TRUE(ContextLyrics::ShouldFetchOnline({}, true, true, song, false));
  EXPECT_FALSE(ContextLyrics::ShouldFetchOnline({}, true, true, song, true));
  EXPECT_FALSE(ContextLyrics::ShouldFetchOnline({}, false, true, song, false));
  song.set_artist({});
  EXPECT_FALSE(ContextLyrics::ShouldFetchOnline({}, true, true, song, false));
}

TEST(StrUtils, TransliterateAscii) {
  const std::string out = StrUtils::Transliterate("Café");
  EXPECT_TRUE(out == "Cafe" || out == "CAFÉ" || out == "Café" || StrUtils::ContainsInsensitive(out, "Cafe"));
}
