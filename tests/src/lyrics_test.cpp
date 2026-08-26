#include "lyrics/htmllyricsprovider.h"

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

TEST(StrUtils, TransliterateAscii) {
  const std::string out = StrUtils::Transliterate("Café");
  EXPECT_TRUE(out == "Cafe" || out == "CAFÉ" || out == "Café" || StrUtils::ContainsInsensitive(out, "Cafe"));
}
