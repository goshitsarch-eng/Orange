#include "dialogs/trackselectionlabels.h"
#include "tagfetcher/musicbrainzclient.h"
#include "tagfetcher/musicbrainzdiscid.h"
#include "device/cddadiscid.h"
#include "device/cddatext.h"
#include "core/networkresume.h"
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

TEST(CddaDiscId, EncodesMusicBrainzTocLikeLibdiscid) {
  EXPECT_EQ(804u, CddaDiscId::TocString(1, 1, 225, {150}).size());
  EXPECT_EQ("CcFXtqAhBZrz0_LQF8dyg5RvOm0-", CddaDiscId::FromOffsets(1, 1, 225, {150}));
  EXPECT_TRUE(CddaDiscId::FromOffsets(0, 0, 0, {}).empty());
}

TEST(CddaText, AppliesAlbumAndReplacesGenericTitle) {
  EXPECT_TRUE(CddaText::TitleIsGeneric("Track 3", 3));
  EXPECT_FALSE(CddaText::TitleIsGeneric("Mysterons", 1));
  Song song(Song::Source::CDDA);
  song.set_title("Track 1");
  song.set_track(1);
  CddaText::Apply(&song, "Dummy", "Portishead", "Mysterons", "Portishead");
  EXPECT_EQ("Dummy", song.album());
  EXPECT_EQ("Portishead", song.albumartist());
  EXPECT_EQ("Mysterons", song.title());
  EXPECT_EQ("Portishead", song.artist());
  SongList incomplete = {song};
  incomplete.front().set_title("Track 1");
  EXPECT_FALSE(CddaText::HasCompleteTitles(incomplete));
  EXPECT_TRUE(CddaText::HasCompleteTitles({song}));
}

TEST(MusicBrainzDiscId, ParsesFirstReleaseTracksLikeQt) {
  EXPECT_EQ("https://musicbrainz.org/ws/2/discid/abc?inc=recordings+artists&fmt=json", MusicBrainzDiscId::DiscUrl("abc"));
  EXPECT_TRUE(MusicBrainzDiscId::ShouldLookup("abc"));
  EXPECT_FALSE(MusicBrainzDiscId::ShouldLookup({}));
  const std::string json = R"json({
    "id": "abc",
    "releases": [{
      "id": "rel-1",
      "title": "Dummy",
      "date": "1994-08-22",
      "artist-credit": [{"name": "Portishead", "artist": {"name": "Portishead"}}],
      "media": [{
        "discs": [{"id": "abc"}],
        "tracks": [
          {"position": 1, "title": "Mysterons", "length": 301000, "artist-credit": [{"name": "Portishead"}],
           "recording": {"id": "rec-1"}},
          {"position": 8, "title": "Roads", "length": 300000, "artist-credit": [{"name": "Portishead"}]}
        ]
      }]
    }]
  })json";
  const auto results = MusicBrainzClient::ParseDiscResults(json, "abc");
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ("Mysterons", results[0].title);
  EXPECT_EQ("Portishead", results[0].artist);
  EXPECT_EQ("Dummy", results[0].album);
  EXPECT_EQ(1994, results[0].year);
  EXPECT_EQ(1, results[0].track);
  EXPECT_EQ("rec-1", results[0].musicbrainz_recording_id);
  EXPECT_EQ("Roads", results[1].title);
  EXPECT_EQ(8, results[1].track);

  SongList cdda;
  for (int track = 1; track <= 8; ++track) {
    Song song(Song::Source::CDDA);
    song.set_title("Track " + std::to_string(track));
    song.set_track(track);
    song.set_valid(true);
    song.set_musicbrainz_disc_id("abc");
    cdda.push_back(song);
  }
  const SongList merged = MusicBrainzDiscId::MergeByTrack(cdda, results);
  EXPECT_EQ("Mysterons", merged[0].title());
  EXPECT_EQ("Roads", merged[7].title());
  EXPECT_EQ("Track 2", merged[1].title());
  EXPECT_EQ("abc", MusicBrainzDiscId::DiscIdFromSongs(merged));
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
  EXPECT_STREQ("Orange was unable to find results for this file", TrackSelectionLabels::UnableToFind());
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

TEST(NetworkResume, ClearsCacheOnlyWhenOnline) {
  EXPECT_TRUE(NetworkResume::ShouldClearConnectionCache(true));
  EXPECT_FALSE(NetworkResume::ShouldClearConnectionCache(false));
  EXPECT_STREQ("network-changed", NetworkResume::kNetworkChangedSignal);
  EXPECT_STREQ("Strawberry/1.2.0 (+https://www.strawberrymusicplayer.org)", NetworkResume::kUserAgent);
}
