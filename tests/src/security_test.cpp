#include "core/httprequestreader.h"
#include "core/oauthstate.h"
#include "engine/alsaplugin.h"
#include "streaming/streamingcoverdownload.h"
#include "utilities/randutils.h"
#include "utilities/safefilename.h"
#include "utilities/urlredact.h"

#include <gtest/gtest.h>

#include <set>

TEST(CryptographicRandomString, ProducesTheRequestedLengthFromTheGivenAlphabet) {
  const std::string value = RandUtils::CryptographicRandomString(44);
  ASSERT_EQ(44u, value.size());
  for (char c : value) {
    EXPECT_TRUE((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) << "unexpected character " << c;
  }
  EXPECT_TRUE(RandUtils::CryptographicRandomString(0).empty());
}

// Not a randomness test -- just that two verifiers in a row are not the same value.
TEST(CryptographicRandomString, DoesNotRepeatItself) {
  std::set<std::string> seen;
  for (int i = 0; i < 32; ++i) {
    seen.insert(RandUtils::CryptographicRandomString(32));
  }
  EXPECT_EQ(32u, seen.size());
}

TEST(OAuthState, OnlyTheStateThatWasSentIsAccepted) {
  const std::string state = OAuthState::Generate();
  ASSERT_EQ(static_cast<size_t>(OAuthState::kLength), state.size());
  EXPECT_TRUE(OAuthState::Matches(state, state));
  EXPECT_FALSE(OAuthState::Matches(state, {}));
  EXPECT_FALSE(OAuthState::Matches(state, state.substr(0, state.size() - 1)));
  EXPECT_FALSE(OAuthState::Matches(state, state.substr(0, state.size() - 1) + "!"));
  // A flow that sent no state has nothing to compare against.
  EXPECT_TRUE(OAuthState::Matches({}, "anything"));
}

TEST(HttpRequestReader, ParsesTheTargetAndQueryOfARequestLine) {
  const std::string line = "GET /callback?code=abc123&state=xyz HTTP/1.1";
  EXPECT_EQ("/callback?code=abc123&state=xyz", HttpRequestReader::TargetFromRequestLine(line));
  const std::string target = HttpRequestReader::TargetFromRequestLine(line);
  EXPECT_EQ("abc123", HttpRequestReader::QueryValue(target, "code"));
  EXPECT_EQ("xyz", HttpRequestReader::QueryValue(target, "state"));
  EXPECT_EQ("", HttpRequestReader::QueryValue(target, "missing"));
  EXPECT_EQ("", HttpRequestReader::QueryValue("/callback", "code"));
  EXPECT_EQ("", HttpRequestReader::TargetFromRequestLine("garbage"));
}

TEST(HttpRequestReader, UnescapesQueryValues) {
  EXPECT_EQ("a b/c", HttpRequestReader::QueryValue("/callback?code=a%20b%2Fc", "code"));
}

TEST(SafeFilename, SeparatorsAndTraversalCannotSurvive) {
  // Separators become underscores and the leading dots are stripped, so nothing is left that a path
  // join could act on.
  EXPECT_EQ("_.._etc_passwd", SafeFilename::Component("../../etc/passwd"));
  EXPECT_EQ("a_b", SafeFilename::Component("a/b"));
  EXPECT_EQ("a_b", SafeFilename::Component("a\\b"));
  EXPECT_EQ("C_temp", SafeFilename::Component("C:temp"));
  EXPECT_EQ("unknown", SafeFilename::Component(".."));
  EXPECT_EQ("unknown", SafeFilename::Component("."));
  EXPECT_EQ("unknown", SafeFilename::Component("..."));
  EXPECT_EQ("unknown", SafeFilename::Component(""));
  EXPECT_EQ("_", SafeFilename::Component("/", "cover"));
  EXPECT_EQ("bashrc", SafeFilename::Component(".bashrc"));
  EXPECT_EQ("name", SafeFilename::Component("  name.  "));
  EXPECT_EQ("a_b", SafeFilename::Component(std::string("a\nb")));
}

TEST(SafeFilename, OrdinaryNamesAreLeftAlone) {
  EXPECT_EQ("Kid A", SafeFilename::Component("Kid A"));
  EXPECT_EQ("Sgt. Pepper", SafeFilename::Component("Sgt. Pepper"));
  EXPECT_EQ("Motörhead", SafeFilename::Component("Motörhead"));
}

TEST(UrlRedact, CredentialsDoNotReachTheLog) {
  EXPECT_EQ("https://music.example.com/rest/stream?c=orange&p=<redacted>&u=<redacted>&v=1.16.1",
            UrlRedact::Sanitize("https://music.example.com/rest/stream?c=orange&p=enc:6869&u=alice&v=1.16.1"));
  EXPECT_EQ("https://music.example.com/rest/stream?s=<redacted>&t=<redacted>",
            UrlRedact::Sanitize("https://music.example.com/rest/stream?s=abcdef&t=deadbeef"));
  // Nothing to redact, nothing changed.
  EXPECT_EQ("https://example.com/song.flac", UrlRedact::Sanitize("https://example.com/song.flac"));
  EXPECT_EQ("https://example.com/a?id=7", UrlRedact::Sanitize("https://example.com/a?id=7"));
}

TEST(StreamingCoverCache, SignedCoverUrlsAreCachedAsTheirBareId) {
  const std::string signed_url = "https://music.example.com/rest/getCoverArt?c=orange&id=al-42&p=enc:6869&u=alice";
  EXPECT_EQ("al-42", StreamingCoverDownload::CoverArtIdFromUrl(signed_url));
  EXPECT_EQ("al-42", StreamingCoverDownload::CacheableCoverArt(signed_url));
  // A plain CDN link has no credentials in it and is cached as it stands.
  EXPECT_EQ("https://cdn.example.com/a.jpg", StreamingCoverDownload::CacheableCoverArt("https://cdn.example.com/a.jpg"));
  EXPECT_EQ("", StreamingCoverDownload::CoverArtIdFromUrl("https://cdn.example.com/a.jpg"));
}

TEST(StreamingCoverCache, RemoteIdsCannotEscapeTheCoverCacheDirectory) {
  EXPECT_EQ("_.._", StreamingCoverDownload::CacheFilename("Subsonic", "../../..", {}));
  EXPECT_EQ("_.._secrets", StreamingCoverDownload::CacheFilename("Subsonic", "../../secrets", {}));
  EXPECT_EQ("al-42", StreamingCoverDownload::CacheFilename("Subsonic", "al-42", {}));
  EXPECT_EQ("", StreamingCoverDownload::CacheFilename("Subsonic", "", {}));
}

#include "playlist/playlistrowindex.h"

// GINT_TO_POINTER(0) is NULL, so a zero-based row index was indistinguishable from "no index set": the
// column header, which carries none, read back as row 0 and inline-editing the first row rewrote it.
TEST(PlaylistRowIndex, RowZeroIsDistinguishableFromAWidgetWithNoIndex) {
  GObject *row = G_OBJECT(g_object_new(G_TYPE_OBJECT, nullptr));
  GObject *header = G_OBJECT(g_object_new(G_TYPE_OBJECT, nullptr));
  PlaylistRowIndex::Set(row, 0);
  EXPECT_EQ(0, PlaylistRowIndex::Get(row));
  EXPECT_EQ(-1, PlaylistRowIndex::Get(header));
  PlaylistRowIndex::Set(row, 41);
  EXPECT_EQ(41, PlaylistRowIndex::Get(row));
  g_object_unref(row);
  g_object_unref(header);
}
