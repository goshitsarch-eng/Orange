#include "dialogs/trackselectionlabels.h"
#include "tagfetcher/musicbrainzclient.h"
#include "core/networktimeoutpolicy.h"
#include "core/networktimeouts.h"
#include "tagfetcher/tagfetchhelpers.h"

#include <glib.h>
#include <gtest/gtest.h>

TEST(MusicBrainzClient, ToSongsPreservesRecordingIds) {
  MusicBrainzClient::Result result;
  result.title = "Ragged Wood";
  result.artist = "Fleet Foxes";
  result.album = "Fleet Foxes";
  result.year = 2008;
  result.duration_msec = 312000;
  result.musicbrainz_recording_id = "rec-1";
  result.musicbrainz_artist_id = "art-1";
  result.musicbrainz_album_id = "rel-1";
  result.musicbrainz_album_artist_id = "aa-1";
  const SongList songs = MusicBrainzClient::ToSongs({result});
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Ragged Wood", songs.front().title());
  EXPECT_EQ("rec-1", songs.front().musicbrainz_recording_id());
  EXPECT_EQ("art-1", songs.front().musicbrainz_artist_id());
  EXPECT_EQ("rel-1", songs.front().musicbrainz_album_id());
  EXPECT_EQ("aa-1", songs.front().musicbrainz_album_artist_id());
  EXPECT_EQ(312000000000, songs.front().length_nanosec());
}

TEST(TagFetchHelpers, ApplyTagsCopiesMetadataOntoOriginal) {
  Song original;
  original.set_title("Old");
  original.set_artist("Local");
  original.set_url("file:///track.flac");
  original.set_id(42);
  original.set_track(3);
  original.set_valid(true);
  Song result;
  result.set_title("Ragged Wood");
  result.set_artist("Fleet Foxes");
  result.set_album("Fleet Foxes");
  result.set_year(2008);
  result.set_musicbrainz_recording_id("rec-1");
  result.set_musicbrainz_artist_id("art-1");
  const Song written = TagFetchHelpers::ApplyTags(original, result);
  EXPECT_EQ("Ragged Wood", written.title());
  EXPECT_EQ("Fleet Foxes", written.artist());
  EXPECT_EQ("Fleet Foxes", written.album());
  EXPECT_EQ(2008, written.year());
  EXPECT_EQ(3, written.track());
  EXPECT_EQ("file:///track.flac", written.url());
  EXPECT_EQ(42, written.id());
  EXPECT_EQ("rec-1", written.musicbrainz_recording_id());
  EXPECT_EQ("art-1", written.musicbrainz_artist_id());
}

TEST(TagFetchHelpers, ApplyResultUsesMusicBrainzFields) {
  Song original;
  original.set_url("file:///keep.mp3");
  original.set_title("Keep");
  MusicBrainzClient::Result result;
  result.title = "White Winter Hymnal";
  result.artist = "Fleet Foxes";
  result.musicbrainz_recording_id = "mbid-wwh";
  const Song written = TagFetchHelpers::ApplyResult(original, result);
  EXPECT_EQ("White Winter Hymnal", written.title());
  EXPECT_EQ("file:///keep.mp3", written.url());
  EXPECT_EQ("mbid-wwh", written.musicbrainz_recording_id());
}

TEST(TagFetchHelpers, ShouldShowDialogIgnoresEmpty) {
  EXPECT_FALSE(TagFetchHelpers::ShouldShowDialog({}));
  EXPECT_TRUE(TagFetchHelpers::ShouldShowDialog("MusicBrainz request failed"));
}

TEST(TagFetchHelpers, BatchProgressTracksCompletion) {
  const auto progress = TagFetchHelpers::BatchProgress::FromCounts(2, 4, 3);
  EXPECT_DOUBLE_EQ(0.5, progress.Fraction());
  EXPECT_FALSE(progress.Done());
  EXPECT_EQ("2 / 4", progress.StatusText());
  EXPECT_TRUE(TagFetchHelpers::BatchProgress::FromCounts(3, 3).Done());
  EXPECT_DOUBLE_EQ(0.0, TagFetchHelpers::BatchProgress::FromCounts(0, 0).Fraction());
}

TEST(MusicBrainzClient, ParseResultsCopiesIdsFromJson) {
  const std::string json = R"json({
    "recordings": [
      {
        "id": "rec-parse",
        "title": "Helplessness Blues",
        "artist-credit": [{"name": "Fleet Foxes", "artist": {"id": "art-parse"}}],
        "releases": [{"id": "rel-parse", "title": "Helplessness Blues", "date": "2011-05-03"}]
      }
    ]
  })json";
  const auto results = MusicBrainzClient::ParseResults(json);
  ASSERT_EQ(1u, results.size());
  EXPECT_EQ("rec-parse", results.front().musicbrainz_recording_id);
  EXPECT_EQ("art-parse", results.front().musicbrainz_artist_id);
  EXPECT_EQ("rel-parse", results.front().musicbrainz_album_id);
}

TEST(TrackSelectionLabels, QtCopyAndEmptyResults) {
  EXPECT_STREQ("Tag fetcher", TrackSelectionLabels::Title());
  EXPECT_STREQ("Select best possible match", TrackSelectionLabels::SelectBest());
  EXPECT_STREQ("No results", TrackSelectionLabels::NoResults());
  EXPECT_STREQ("Strawberry was unable to find results for this file", TrackSelectionLabels::UnableToFind());
  EXPECT_STREQ("Error", TrackSelectionLabels::Error());
  EXPECT_STREQ("Original tags", TrackSelectionLabels::OriginalTags());
  EXPECT_STREQ("Saving tracks...", TrackSelectionLabels::SavingTracks());
  EXPECT_FALSE(TrackSelectionLabels::ButtonsEnabled(true));
  EXPECT_TRUE(TrackSelectionLabels::ButtonsEnabled(false));
  EXPECT_FALSE(TrackSelectionLabels::SplitterEnabled(true));
  EXPECT_TRUE(TrackSelectionLabels::SplitterEnabled(false));
  EXPECT_TRUE(TrackSelectionLabels::LoadingVisible(true));
  EXPECT_FALSE(TrackSelectionLabels::LoadingVisible(false));
  EXPECT_TRUE(TrackSelectionLabels::ShowEmptyResults(false, false));
  EXPECT_FALSE(TrackSelectionLabels::ShowEmptyResults(true, false));
  EXPECT_FALSE(TrackSelectionLabels::ShowEmptyResults(false, true));
  EXPECT_FALSE(TrackSelectionLabels::SongListVisible(0));
  EXPECT_FALSE(TrackSelectionLabels::SongListVisible(1));
  EXPECT_TRUE(TrackSelectionLabels::SongListVisible(2));
  EXPECT_FALSE(TrackSelectionLabels::NavEnabled(1));
  EXPECT_TRUE(TrackSelectionLabels::NavEnabled(3));
  EXPECT_FALSE(TrackSelectionLabels::ApplyAllVisible(1));
  EXPECT_TRUE(TrackSelectionLabels::ApplyAllVisible(2));
}

TEST(NetworkTimeouts, AbortFiresAfterTimeout) {
  NetworkTimeouts timeouts;
  timeouts.SetTimeout(1);
  int aborted = -1;
  timeouts.SetAbort([&aborted](int id) { aborted = id; });
  timeouts.AddReply(42);
  EXPECT_TRUE(timeouts.Contains(42));
  const gint64 deadline = g_get_monotonic_time() + 500 * G_TIME_SPAN_MILLISECOND;
  while (aborted < 0 && g_get_monotonic_time() < deadline) {
    g_main_context_iteration(nullptr, TRUE);
  }
  EXPECT_EQ(42, aborted);
  EXPECT_FALSE(timeouts.Contains(42));
}

TEST(NetworkTimeouts, CancelPreventsAbort) {
  NetworkTimeouts timeouts;
  timeouts.SetTimeout(50);
  bool aborted = false;
  timeouts.SetAbort([&aborted](int) { aborted = true; });
  timeouts.AddReply(7);
  timeouts.Cancel(7);
  EXPECT_FALSE(timeouts.Contains(7));
  const gint64 deadline = g_get_monotonic_time() + 80 * G_TIME_SPAN_MILLISECOND;
  while (g_get_monotonic_time() < deadline) {
    g_main_context_iteration(nullptr, FALSE);
    g_usleep(1000);
  }
  EXPECT_FALSE(aborted);
}

TEST(NetworkTimeouts, AddReplyZeroIsIgnored) {
  NetworkTimeouts timeouts;
  timeouts.SetTimeout(1);
  timeouts.AddReply(0);
  EXPECT_FALSE(timeouts.Contains(0));
}
