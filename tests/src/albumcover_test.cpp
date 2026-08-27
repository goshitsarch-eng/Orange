#include "collection/collectionalbumart.h"
#include "playlist/playlistcoverpersist.h"
#include "covermanager/coverchoicemenu.h"
#include "covermanager/covermanageractivate.h"
#include "covermanager/covermanageractions.h"
#include "covermanager/covermanagermenu.h"
#include "covermanager/covermanagerstats.h"
#include "covermanager/covermanagerview.h"
#include "covermanager/albumcoverbatch.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverexportlabels.h"
#include "covermanager/coverarttypes.h"
#include "covermanager/covermanagerexportscope.h"
#include "covermanager/albumcoverexporter.h"
#include "covermanager/coverexportjob.h"
#include "covermanager/albumcoverfetcher.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/albumcoversearcher.h"
#include "covermanager/albumcoversearcherlabels.h"
#include "covermanager/albumcoverloader.h"
#include "covermanager/covererrormessage.h"
#include "dialogs/uierror.h"
#include "covermanager/albumcoverloaderoptions.h"
#include "covermanager/coverexportrunnable.h"
#include "covermanager/coversearchstatistics.h"
#include "covermanager/coversearchstatisticsdialog.h"
#include "covermanager/coversearchstatisticslabels.h"
#include "covermanager/coverfetchpolicy.h"
#include "covermanager/coverprovidersettings.h"
#include "covermanager/currentalbumcoverloader.h"
#include "settings/coverssettingslabels.h"
#include "utilities/fileutils.h"

#include <glib/gstdio.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

TEST(CoverSearchStatisticsDialog, SummaryIncludesCounts) {
  CoverSearchStatistics stats;
  stats.network_requests_made = 3;
  stats.chosen_images = 2;
  stats.missing_images = 1;
  stats.total_images_by_provider["Last.fm"] = 4;
  stats.chosen_images_by_provider["Last.fm"] = 2;
  const std::string text = CoverSearchStatisticsDialog::SummaryText(stats);
  EXPECT_EQ("Got 2 covers out of 3 (1 failed)", CoverSearchStatisticsLabels::Got(2, 1));
  EXPECT_STREQ("Fetch completed", CoverSearchStatisticsLabels::Title());
  EXPECT_NE(std::string::npos, text.find("Got 2 covers out of 3 (1 failed)"));
  EXPECT_NE(std::string::npos, text.find("Covers from Last.fm: 2"));
  EXPECT_NE(std::string::npos, text.find("Total network requests made: 3"));
  EXPECT_NE(std::string::npos, text.find("0 bytes"));
}

TEST(CoverSearchStatistics, AverageDimensionsAndAccumulate) {
  CoverSearchStatistics empty;
  EXPECT_EQ("0x0", empty.AverageDimensions());

  CoverSearchStatistics a;
  a.chosen_images = 2;
  a.chosen_width = 1000;
  a.chosen_height = 800;
  a.total_images_by_provider["Last.fm"] = 3;
  a.chosen_images_by_provider["Last.fm"] = 1;
  a.network_requests_made = 4;
  EXPECT_EQ("500x400", a.AverageDimensions());

  CoverSearchStatistics b;
  b.chosen_images = 1;
  b.chosen_width = 200;
  b.chosen_height = 100;
  b.total_images_by_provider["Last.fm"] = 1;
  b.total_images_by_provider["Deezer"] = 2;
  a += b;
  EXPECT_EQ(3u, a.chosen_images);
  EXPECT_EQ(4u, a.total_images_by_provider["Last.fm"]);
  EXPECT_EQ(2u, a.total_images_by_provider["Deezer"]);
  EXPECT_EQ("400x300", a.AverageDimensions());
}

TEST(CollectionAlbumArt, PropagatesCollectionNotRadio) {
  EXPECT_TRUE(CollectionAlbumArt::ShouldPropagate(Song::Source::Collection));
  EXPECT_TRUE(CollectionAlbumArt::ShouldPropagate(Song::Source::Device));
  EXPECT_FALSE(CollectionAlbumArt::ShouldPropagate(Song::Source::SomaFM));
  Song song;
  song.set_album("Dummy");
  song.set_artist("Portishead");
  EXPECT_TRUE(CollectionAlbumArt::AlbumKeyValid(song));
  song.set_album({});
  EXPECT_FALSE(CollectionAlbumArt::AlbumKeyValid(song));
}

TEST(CoverFetchPolicy, StopsFetchWhenScoreIsGood) {
  EXPECT_FLOAT_EQ(4.0f, AlbumCoverFetcherSearch::kGoodScore);
  EXPECT_TRUE(CoverFetchPolicy::ShouldStop(4.0f, false));
  EXPECT_TRUE(CoverFetchPolicy::ShouldStop(5.0f, false));
  EXPECT_FALSE(CoverFetchPolicy::ShouldStop(3.9f, false));
  EXPECT_FALSE(CoverFetchPolicy::ShouldStop(5.0f, true));
}

TEST(AlbumCoverFetcherSearch, ScoreImageMatchesOriginalFormula) {
  EXPECT_FLOAT_EQ(0.0f, AlbumCoverFetcherSearch::ScoreImage(0, 500));
  EXPECT_FLOAT_EQ(2.0f, AlbumCoverFetcherSearch::ScoreImage(500, 500));
  const float wide = AlbumCoverFetcherSearch::ScoreImage(1000, 500);
  EXPECT_GT(wide, 1.0f);
  EXPECT_LT(wide, AlbumCoverFetcherSearch::ScoreImage(1000, 1000));
}

TEST(AlbumCoverFetcherSearch, ScoresExactMatchAndCompilationPenalty) {
  CoverSearchRequest request;
  request.artist = "Fleet Foxes";
  request.album = "Helplessness Blues";

  CoverProviderSearchResults results;
  CoverProviderSearchResult exact;
  exact.artist = "Fleet Foxes";
  exact.album = "Helplessness Blues";
  exact.image_width = 500;
  exact.image_height = 500;
  CoverProviderSearchResult other;
  other.artist = "Someone Else";
  other.album = "Another Album";
  results.push_back(exact);
  results.push_back(other);
  AlbumCoverFetcherSearch::ScoreResults(request, 1.5f, "MusicBrainz", &results);
  EXPECT_EQ("MusicBrainz", results[0].provider);
  EXPECT_FLOAT_EQ(1.5f, results[0].score_provider);
  EXPECT_FLOAT_EQ(1.0f, results[0].score_match);
  EXPECT_FLOAT_EQ(2.0f, results[0].score_quality);
  EXPECT_FLOAT_EQ(-1.5f, results[1].score_match);
  AlbumCoverFetcherSearch::SortByScore(&results);
  EXPECT_EQ("Helplessness Blues", results.front().album);

  CoverSearchRequest untitled;
  untitled.artist = "Fleet Foxes";
  CoverProviderSearchResult live;
  live.artist = "Fleet Foxes";
  live.album = "Live in Seattle";
  CoverProviderSearchResults live_results = {live};
  AlbumCoverFetcherSearch::ScoreResults(untitled, 1.0f, "Last.fm", &live_results);
  EXPECT_FLOAT_EQ(-0.5f, live_results[0].score_match);
  EXPECT_TRUE(AlbumCoverFetcherSearch::IsCompilationOrLiveAlbum("Greatest Hits"));
  EXPECT_TRUE(AlbumCoverFetcherSearch::IsCompilationOrLiveAlbum("The Essential Collection"));
  EXPECT_FALSE(AlbumCoverFetcherSearch::IsCompilationOrLiveAlbum("Helplessness Blues"));
}

TEST(CoverArtTypes, ParseSaveAndFilenameSensitivity) {
  EXPECT_EQ("art_unset", CoverArtTypes::AllIds().front());
  EXPECT_EQ("art_embedded,art_automatic,art_manual", CoverArtTypes::DefaultSaved());
  EXPECT_EQ("Embedded album cover art (art_embedded)", CoverArtTypes::Description("art_embedded"));
  EXPECT_EQ("Manually unset (art_unset)", CoverArtTypes::Description("art_unset"));
  EXPECT_EQ("Set through album cover search (art_manual)", CoverArtTypes::Description("art_manual"));
  EXPECT_EQ("Automatically picked up from album directory (art_automatic)", CoverArtTypes::Description("art_automatic"));

  const auto missing_unset = CoverArtTypes::Parse(CoverArtTypes::DefaultSaved());
  ASSERT_EQ(4u, missing_unset.size());
  EXPECT_EQ("art_embedded", missing_unset[0].id);
  EXPECT_TRUE(missing_unset[0].enabled);
  EXPECT_EQ("art_automatic", missing_unset[1].id);
  EXPECT_TRUE(missing_unset[1].enabled);
  EXPECT_EQ("art_manual", missing_unset[2].id);
  EXPECT_TRUE(missing_unset[2].enabled);
  EXPECT_EQ("art_unset", missing_unset[3].id);
  EXPECT_FALSE(missing_unset[3].enabled);
  EXPECT_EQ(CoverArtTypes::DefaultSaved(), CoverArtTypes::Save(missing_unset));

  const auto qt_default = CoverArtTypes::Parse("art_unset,art_manual,art_automatic,art_embedded");
  ASSERT_EQ(4u, qt_default.size());
  EXPECT_EQ("art_unset", qt_default[0].id);
  EXPECT_TRUE(qt_default[0].enabled);
  EXPECT_EQ("art_embedded,art_automatic,art_manual", CoverArtTypes::Save(CoverArtTypes::Move(missing_unset, 0, 0)));
  const auto moved = CoverArtTypes::Move(missing_unset, 0, 1);
  EXPECT_EQ("art_automatic", moved[0].id);
  EXPECT_EQ("art_embedded", moved[1].id);

  const auto all_disabled = CoverArtTypes::Parse("");
  EXPECT_TRUE(CoverArtTypes::EnabledIds(all_disabled).empty());
  EXPECT_TRUE(CoverArtTypes::Save(all_disabled).empty());
  EXPECT_EQ(4u, all_disabled.size());
  EXPECT_FALSE(CoverArtTypes::Parse("art_unknown,art_embedded")[0].id.empty());
  EXPECT_EQ("art_embedded", CoverArtTypes::Parse("art_unknown,art_embedded").front().id);

  EXPECT_TRUE(CoverArtTypes::FilenameGroupEnabled("2"));
  EXPECT_TRUE(CoverArtTypes::FilenameGroupEnabled(CoverOptions::CoverType::Album));
  EXPECT_FALSE(CoverArtTypes::FilenameGroupEnabled("1"));
  EXPECT_FALSE(CoverArtTypes::FilenameGroupEnabled("3"));
  EXPECT_TRUE(CoverArtTypes::FilenamePatternOptionsEnabled("2", "2"));
  EXPECT_FALSE(CoverArtTypes::FilenamePatternOptionsEnabled("2", "1"));
  EXPECT_FALSE(CoverArtTypes::FilenamePatternOptionsEnabled("1", "2"));
  EXPECT_FALSE(CoverArtTypes::FilenamePatternOptionsEnabled(CoverOptions::CoverType::Cache, CoverOptions::CoverFilename::Pattern));
}

TEST(AlbumCoverLoaderOptions, TypeNamesAndLoadTypesDefault) {
  EXPECT_EQ("art_embedded", AlbumCoverLoaderOptions::TypeName(AlbumCoverLoaderOptions::Type::Embedded));
  EXPECT_EQ(AlbumCoverLoaderOptions::Type::Manual, AlbumCoverLoaderOptions::TypeFromName("art_manual"));
  const auto types = AlbumCoverLoaderOptions::LoadTypes();
  ASSERT_FALSE(types.empty());
  EXPECT_EQ(AlbumCoverLoaderOptions::Type::Embedded, types.front());
}

TEST(AlbumCoverLoader, FindsFolderCoverAndCurrentLoader) {
  const std::string root = "/tmp/strawberry-cover-" + std::to_string(getpid());
  g_mkdir_with_parents(root.c_str(), 0755);
  const std::string song_path = FileUtils::Join(root, "track.mp3");
  const std::string cover_path = FileUtils::Join(root, "cover.jpg");
  FileUtils::WriteFile(song_path, "x");
  FileUtils::WriteFile(cover_path, "cover-bytes");

  Song song;
  song.set_url(FileUtils::UriFromPath(song_path));
  AlbumCoverLoader loader(nullptr);
  const std::string found = loader.LoadPath(song);
  EXPECT_NE(std::string::npos, found.find("cover.jpg"));
  const auto data = loader.LoadData(song);
  EXPECT_EQ(std::string(data.begin(), data.end()), "cover-bytes");

  CurrentAlbumCoverLoader current(&loader);
  current.Load(song);
  EXPECT_EQ(current.current(), data);

  FileUtils::Remove(song_path);
  FileUtils::Remove(cover_path);
}

TEST(AlbumCoverExport, DialogResultFlags) {
  AlbumCoverExport::DialogResult result;
  EXPECT_FALSE(result.IsSizeForced());
  EXPECT_FALSE(result.RequiresCoverProcessing());
  result.forcesize = true;
  result.width = 300;
  result.height = 300;
  EXPECT_TRUE(result.IsSizeForced());
  EXPECT_TRUE(result.RequiresCoverProcessing());
  result.forcesize = false;
  result.overwrite = AlbumCoverExport::OverwriteMode::Smaller;
  EXPECT_TRUE(result.RequiresCoverProcessing());
}

TEST(AlbumCoverExportLabels, MatchQtExportDialog) {
  EXPECT_STREQ("Export covers", AlbumCoverExportLabels::Title());
  EXPECT_STREQ("Enter a filename for exported covers (no extension):", AlbumCoverExportLabels::FilenamePrompt());
  EXPECT_STREQ("Export downloaded covers", AlbumCoverExportLabels::ExportDownloaded());
  EXPECT_STREQ("Export embedded covers", AlbumCoverExportLabels::ExportEmbedded());
  EXPECT_STREQ("Do not overwrite", AlbumCoverExportLabels::DoNotOverwrite());
  EXPECT_STREQ("Overwrite all", AlbumCoverExportLabels::OverwriteAll());
  EXPECT_STREQ("Overwrite smaller ones only", AlbumCoverExportLabels::OverwriteSmaller());
  EXPECT_STREQ("Scale size", AlbumCoverExportLabels::ScaleSize());
  EXPECT_STREQ("cover", AlbumCoverExportLabels::DefaultFilename());
  EXPECT_STREQ("Do not overwrite", AlbumCoverExportLabels::OverwriteLabel(AlbumCoverExport::OverwriteMode::None));
  EXPECT_EQ(AlbumCoverExport::OverwriteMode::Smaller, AlbumCoverExportLabels::OverwriteFromInt(2));
  EXPECT_EQ(0, AlbumCoverExportLabels::OverwriteRadioIndex(AlbumCoverExport::OverwriteMode::None));
  EXPECT_FALSE(AlbumCoverExportLabels::ForceSizeEnabled(false));
  EXPECT_TRUE(AlbumCoverExportLabels::ForceSizeEnabled(true));
  const AlbumCoverExport::DialogResult defaults = AlbumCoverExportLabels::Defaults();
  EXPECT_TRUE(defaults.export_downloaded);
  EXPECT_FALSE(defaults.export_embedded);
  const auto types = AlbumCoverExportLabels::TypesFor(defaults);
  ASSERT_EQ(2u, types.size());
  EXPECT_EQ(AlbumCoverLoaderOptions::Type::Automatic, types[0]);
  EXPECT_EQ(AlbumCoverLoaderOptions::Type::Manual, types[1]);
  AlbumCoverExport::DialogResult embedded = defaults;
  embedded.export_downloaded = false;
  embedded.export_embedded = true;
  const auto embedded_types = AlbumCoverExportLabels::TypesFor(embedded);
  ASSERT_EQ(1u, embedded_types.size());
  EXPECT_EQ(AlbumCoverLoaderOptions::Type::Embedded, embedded_types[0]);
}

TEST(CoverManagerExportScope, VisibleAlbumsWithCoversOnly) {
  EXPECT_STREQ("No covers to export.", CoverManagerExportScope::NoCoversText());
  EXPECT_STREQ("Export finished", CoverManagerExportScope::FinishedTitle());
  EXPECT_EQ("Exported 3 covers (1 skipped).", CoverManagerExportScope::FinishedBody(3, 1));
  AlbumCoverManagerList::Album covered;
  covered.album = "Dummy";
  covered.has_cover = true;
  covered.song.set_title("Track");
  AlbumCoverManagerList::Album bare;
  bare.album = "Bare";
  bare.has_cover = false;
  bare.song.set_title("Other");
  const SongList songs = CoverManagerExportScope::SongsToExport({covered, bare});
  ASSERT_EQ(1u, songs.size());
  EXPECT_EQ("Track", songs.front().title());
  EXPECT_TRUE(CoverManagerExportScope::SongsToExport({bare}).empty());
}

TEST(CoverExportRunnable, CopiesManualCoverAndSkipsExisting) {
  const std::string root = "/tmp/strawberry-export-" + std::to_string(getpid());
  g_mkdir_with_parents(root.c_str(), 0755);
  const std::string song_path = FileUtils::Join(root, "track.mp3");
  const std::string source = FileUtils::Join(root, "front.png");
  FileUtils::WriteFile(song_path, "x");
  FileUtils::WriteFile(source, "png-bytes");

  Song song;
  song.set_url(FileUtils::UriFromPath(song_path));
  song.set_art_manual(FileUtils::UriFromPath(source));

  AlbumCoverExport::DialogResult dialog;
  dialog.export_downloaded = true;
  dialog.filename = "cover";
  dialog.overwrite = AlbumCoverExport::OverwriteMode::None;
  EXPECT_EQ(FileUtils::Join(root, "cover.png"), CoverExportRunnable::DestinationPath(song, dialog, "png"));

  CoverExportRunnable job(nullptr, dialog, {AlbumCoverLoaderOptions::Type::Manual}, song);
  EXPECT_TRUE(job.Run());
  EXPECT_EQ("png-bytes", FileUtils::ReadFile(FileUtils::Join(root, "cover.png")));
  EXPECT_FALSE(job.Run());

  AlbumCoverExporter exporter(nullptr);
  exporter.SetDialogResult(dialog);
  exporter.SetCoverTypes({AlbumCoverLoaderOptions::Type::Manual});
  exporter.AddExportRequest(song);
  exporter.StartExporting();
  EXPECT_EQ(1, exporter.request_count());
  EXPECT_EQ(0, exporter.exported());
  EXPECT_EQ(1, exporter.skipped());
  EXPECT_TRUE(exporter.finished());

  EXPECT_EQ(3, CoverExportJob::kMaxConcurrent);
  EXPECT_TRUE(CoverExportJob::ShouldPump(2, 1));
  EXPECT_FALSE(CoverExportJob::ShouldPump(3, 1));
  EXPECT_DOUBLE_EQ(0.5, CoverExportJob::ProgressFraction(1, 1, 4));
  EXPECT_EQ("Exported 1 of 4 (1 skipped)", CoverExportJob::StatusText(1, 1, 4));
  EXPECT_TRUE(CoverExportJob::ShouldFinish(4, 4, false));
  EXPECT_FALSE(CoverExportJob::ShouldScheduleNext(0, 4, false, false));

  AlbumCoverExporter batched(nullptr);
  batched.SetDialogResult(dialog);
  batched.SetCoverTypes({AlbumCoverLoaderOptions::Type::Manual});
  for (int i = 0; i < 4; ++i) {
    Song extra;
    extra.set_url(FileUtils::UriFromPath(FileUtils::Join(root, "missing-" + std::to_string(i) + ".mp3")));
    batched.AddExportRequest(extra);
  }
  batched.ProcessSome();
  EXPECT_EQ(3, batched.next_index());
  EXPECT_FALSE(batched.finished());
  EXPECT_EQ(3, batched.skipped());
  batched.ProcessSome();
  EXPECT_TRUE(batched.finished());
  EXPECT_EQ(4, batched.skipped());

  FileUtils::Remove(FileUtils::Join(root, "cover.png"));
  FileUtils::Remove(source);
  FileUtils::Remove(song_path);
}

TEST(AlbumCoverFetcher, IncrementsRequestIds) {
  AlbumCoverFetcher fetcher(nullptr, nullptr);
  EXPECT_EQ(1u, fetcher.SearchForCovers("A", "B"));
  EXPECT_EQ(2u, fetcher.FetchAlbumCover("A", "B", "C", true));
  EXPECT_EQ(3u, fetcher.next_id());
  EXPECT_EQ(0u, fetcher.queued());
  EXPECT_EQ(0u, fetcher.active());
  fetcher.Clear();
  EXPECT_EQ(0u, fetcher.queued());
}

TEST(AlbumCoverFetcherSearch, RequestHitsAndStatus) {
  const CoverSearchRequest request = AlbumCoverFetcherSearch::MakeRequest(7, "Portishead", "Dummy (Disc 1)", "Roads", true, false);
  EXPECT_EQ(7u, request.id);
  EXPECT_EQ("Portishead", request.artist);
  EXPECT_EQ("Dummy", request.album);
  EXPECT_EQ("Roads", request.title);
  EXPECT_TRUE(request.search);
  EXPECT_FALSE(request.batch);

  const Song song = AlbumCoverFetcherSearch::SongFromRequest(request);
  EXPECT_EQ("Portishead", song.EffectiveAlbumartist());
  EXPECT_EQ("Dummy", song.album());
  EXPECT_EQ("Roads", song.title());

  CoverProviderSearchResults results = {
      AlbumCoverFetcherSearch::FromHit("Last.fm", "Portishead", "Dummy", "https://example/a.jpg", 500, 500),
      AlbumCoverFetcherSearch::FromHit("Deezer", "Other", "Other", "https://example/b.jpg", 100, 100),
  };
  AlbumCoverFetcherSearch::ScoreResults(request, 1.5f, "Last.fm", &results);
  AlbumCoverFetcherSearch::SortByScore(&results);
  AlbumCoverFetcherSearch::AssignNumbers(&results);
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ(1, results.front().number);
  EXPECT_EQ("Portishead - Dummy", AlbumCoverFetcherSearch::ResultLabel(results.front()));
  EXPECT_NE(std::string::npos, AlbumCoverFetcherSearch::ResultSubtitle(results.front()).find("Last.fm"));
  EXPECT_EQ(&results.front(), AlbumCoverFetcherSearch::Best(results));
  EXPECT_TRUE(AlbumCoverFetcherSearch::IsHttpUrl("https://example/a.jpg"));
  EXPECT_FALSE(AlbumCoverFetcherSearch::IsHttpUrl("not-a-url"));
  EXPECT_EQ("Searching providers for “Dummy”…", AlbumCoverFetcherSearch::StatusSearching("Dummy"));
  EXPECT_EQ("No covers found", AlbumCoverFetcherSearch::StatusFound(0));
  EXPECT_EQ("1 cover found", AlbumCoverFetcherSearch::StatusFound(1));
  EXPECT_EQ("2 covers found", AlbumCoverFetcherSearch::StatusFound(2));
}

TEST(AlbumCoverSearcherLabels, QtCopy) {
  EXPECT_STREQ("Cover Manager", AlbumCoverSearcherLabels::Title());
  EXPECT_STREQ("Artist", AlbumCoverSearcherLabels::Artist());
  EXPECT_STREQ("Album", AlbumCoverSearcherLabels::Album());
  EXPECT_STREQ("Search", AlbumCoverSearcherLabels::Search());
  EXPECT_STREQ("Abort", AlbumCoverSearcherLabels::Abort());
}

TEST(AlbumCoverSearcher, SearchLockAndAbort) {
  EXPECT_STREQ("Search", AlbumCoverSearcher::SearchButtonLabel(false));
  EXPECT_STREQ("Abort", AlbumCoverSearcher::SearchButtonLabel(true));
  EXPECT_TRUE(AlbumCoverSearcher::FieldsEnabled(false));
  EXPECT_FALSE(AlbumCoverSearcher::FieldsEnabled(true));
  EXPECT_TRUE(AlbumCoverSearcher::GridEnabled(false));
  EXPECT_FALSE(AlbumCoverSearcher::GridEnabled(true));
  EXPECT_FALSE(AlbumCoverSearcher::BusyVisible(false));
  EXPECT_TRUE(AlbumCoverSearcher::BusyVisible(true));
  EXPECT_TRUE(AlbumCoverSearcher::ShouldStartSearch(false));
  EXPECT_FALSE(AlbumCoverSearcher::ShouldStartSearch(true));
  EXPECT_FALSE(AlbumCoverSearcher::ShouldAbortSearch(false));
  EXPECT_TRUE(AlbumCoverSearcher::ShouldAbortSearch(true));
  EXPECT_FALSE(AlbumCoverSearcher::ShouldAutoSearch("", ""));
  EXPECT_TRUE(AlbumCoverSearcher::ShouldAutoSearch("Portishead", ""));
  EXPECT_TRUE(AlbumCoverSearcher::ShouldAutoSearch("", "Dummy"));
}

TEST(AlbumCoverSearcher, IgnoresEnterLikeQt) {
  EXPECT_TRUE(AlbumCoverSearcher::ShouldIgnoreEnter(ListBoxKeyboard::kReturn));
  EXPECT_TRUE(AlbumCoverSearcher::ShouldIgnoreEnter(ListBoxKeyboard::kKPEnter));
  EXPECT_FALSE(AlbumCoverSearcher::ShouldIgnoreEnter(ListBoxKeyboard::kEscape));
  EXPECT_FALSE(AlbumCoverSearcher::ShouldIgnoreEnter('a'));
}

TEST(AlbumCoverSearcher, GridHelpers) {
  EXPECT_EQ(3, AlbumCoverSearcher::ColumnsForWidth(0));
  EXPECT_EQ(2, AlbumCoverSearcher::ColumnsForWidth(100));
  EXPECT_EQ(4, AlbumCoverSearcher::ColumnsForWidth(4 * (AlbumCoverSearcher::kIconSize + AlbumCoverSearcher::kCellPadding)));
  EXPECT_EQ(6, AlbumCoverSearcher::ColumnsForWidth(2000));
  EXPECT_TRUE(AlbumCoverSearcher::DimensionLabel(0, 500).empty());
  EXPECT_EQ("640×640", AlbumCoverSearcher::DimensionLabel(640, 640));
  CoverProviderSearchResult result = AlbumCoverFetcherSearch::FromHit("Last.fm", "Portishead", "Dummy", "https://example/a.jpg", 500, 500);
  EXPECT_EQ("Last.fm · 500×500", AlbumCoverSearcher::CellSubtitle(result));
  EXPECT_TRUE(AlbumCoverSearcher::CanLoadThumb(result));
  CoverProviderSearchResult empty;
  EXPECT_FALSE(AlbumCoverSearcher::CanLoadThumb(empty));
}

TEST(AlbumCoverBatch, ProgressAbortAndStatus) {
  AlbumCoverBatch batch;
  EXPECT_TRUE(batch.finished() || !batch.started());
  EXPECT_EQ("No albums to fetch.", batch.StatusText());
  EXPECT_DOUBLE_EQ(1.0, batch.Progress());

  AlbumCoverBatch::Job first;
  first.artist = "Portishead";
  first.album = "Dummy";
  first.song.set_title("Roads");
  AlbumCoverBatch::Job second;
  second.artist = "Fleet Foxes";
  second.album = "Helplessness Blues";
  batch.Enqueue(first);
  batch.Enqueue(second);
  EXPECT_EQ(2u, batch.total());
  EXPECT_FALSE(batch.started());
  batch.Start();
  EXPECT_TRUE(batch.running());
  ASSERT_TRUE(batch.Current());
  EXPECT_EQ("Portishead", batch.Current()->artist);
  batch.MarkSuccess();
  EXPECT_EQ(1u, batch.succeeded());
  EXPECT_EQ(1u, batch.remaining());
  EXPECT_DOUBLE_EQ(0.5, batch.Progress());
  EXPECT_NE(std::string::npos, batch.StatusText().find("1/2"));
  batch.MarkFailure();
  EXPECT_TRUE(batch.finished());
  EXPECT_EQ(1u, batch.failed());
  EXPECT_NE(std::string::npos, batch.StatusText().find("finished"));

  batch.Reset();
  batch.Enqueue(first);
  batch.Start();
  batch.Cancel();
  EXPECT_TRUE(batch.cancelled());
  EXPECT_TRUE(batch.finished());
  EXPECT_NE(std::string::npos, batch.StatusText().find("cancelled"));
}

TEST(CoverChoiceMenu, ItemsAndWhenToShow) {
  const auto items = CoverChoiceMenu::Items();
  ASSERT_EQ(9u, items.size());
  EXPECT_EQ(9, CoverChoiceMenu::ItemCount());
  EXPECT_EQ("Show cover", items.front().label);
  EXPECT_EQ("show", items.front().id);
  EXPECT_EQ(CoverChoiceMenu::Action::Show, items.front().action);
  EXPECT_EQ("Delete cover", items.back().label);
  EXPECT_EQ(CoverChoiceMenu::Action::Search, CoverChoiceMenu::FromId("search"));
  EXPECT_EQ(CoverChoiceMenu::Action::Fetch, CoverChoiceMenu::FromId("fetch"));
  EXPECT_EQ(CoverChoiceMenu::Action::Show, CoverChoiceMenu::FromId("nope"));
  EXPECT_EQ("cover.save", CoverChoiceMenu::ActionPath("cover", "save"));
  EXPECT_TRUE(CoverChoiceMenu::HasCoverActions(true, true));
  EXPECT_FALSE(CoverChoiceMenu::HasCoverActions(true, false));
  EXPECT_FALSE(CoverChoiceMenu::HasCoverActions(false, true));
  EXPECT_STREQ("Fetch automatically", CoverChoiceMenu::SearchAutomaticallyLabel());
  EXPECT_STREQ("auto", CoverChoiceMenu::SearchAutomaticallyId());
  EXPECT_EQ("cover.auto", CoverChoiceMenu::SearchAutomaticallyPath("cover"));
  EXPECT_TRUE(CoverChoiceMenu::IsSearchAutomatically("auto"));
  EXPECT_FALSE(CoverChoiceMenu::IsSearchAutomatically("search"));
  EXPECT_EQ(10, CoverChoiceMenu::ItemCountWithAutoSearch());
  EXPECT_EQ(9, CoverChoiceMenu::ItemCount());
}

TEST(CoverManagerMenu, UsesCoverChoiceAndPlaylist) {
  const auto covers = CoverManagerMenu::CoverItems();
  const auto playlist = CoverManagerMenu::PlaylistItems();
  ASSERT_EQ(9u, covers.size());
  ASSERT_EQ(2u, playlist.size());
  EXPECT_EQ(11, CoverManagerMenu::ItemCount());
  EXPECT_TRUE(CoverManagerMenu::HasSearch());
  EXPECT_TRUE(CoverManagerMenu::IsCoverId("search"));
  EXPECT_TRUE(CoverManagerMenu::IsCoverId("fetch"));
  EXPECT_TRUE(CoverManagerMenu::IsCoverId("show"));
  EXPECT_FALSE(CoverManagerMenu::IsCoverId("append"));
  EXPECT_TRUE(CoverManagerMenu::IsPlaylistId("append"));
  EXPECT_TRUE(CoverManagerMenu::IsPlaylistId("load"));
  EXPECT_FALSE(CoverManagerMenu::IsPlaylistId("search"));
  EXPECT_EQ("Add to playlist", playlist.front().label);
  EXPECT_EQ("Load to playlist", playlist.back().label);
  EXPECT_FALSE(CoverManagerMenu::LoadReplacesPlaylist("append"));
  EXPECT_TRUE(CoverManagerMenu::LoadReplacesPlaylist("load"));
  EXPECT_EQ(CoverChoiceMenu::Action::Search, CoverChoiceMenu::FromId("search"));
}

TEST(CoverManagerMenu, VisibleItemsFollowCoverState) {
  EXPECT_FALSE(CoverManagerMenu::HasAnyProviders(0));
  EXPECT_TRUE(CoverManagerMenu::HasAnyProviders(2));
  EXPECT_TRUE(CoverManagerMenu::VisibleCoverItems(CoverManagerMenu::Analyze({})).empty());
  EXPECT_FALSE(CoverManagerMenu::IncludePlaylistItems(CoverManagerMenu::Analyze({})));

  Song covered;
  covered.set_valid(true);
  covered.set_art_embedded(true);
  const auto with_cover = CoverManagerMenu::VisibleCoverItems(CoverManagerMenu::FromSong(covered));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Show));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Save));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Delete));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Unset));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Clear));
  EXPECT_TRUE(CoverManagerMenu::Contains(with_cover, CoverChoiceMenu::Action::Search));
  EXPECT_TRUE(CoverManagerMenu::IncludePlaylistItems(CoverManagerMenu::FromSong(covered)));

  Song clear;
  clear.set_valid(true);
  EXPECT_TRUE(CoverManagerMenu::IsClear(clear));
  const auto no_cover = CoverManagerMenu::VisibleCoverItems(CoverManagerMenu::FromSong(clear));
  EXPECT_FALSE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Show));
  EXPECT_FALSE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Save));
  EXPECT_FALSE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Delete));
  EXPECT_TRUE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Unset));
  EXPECT_FALSE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Clear));
  EXPECT_TRUE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::File));
  EXPECT_TRUE(CoverManagerMenu::Contains(no_cover, CoverChoiceMenu::Action::Fetch));

  Song unset;
  unset.set_valid(true);
  unset.set_art_unset(true);
  const auto unset_items = CoverManagerMenu::VisibleCoverItems(CoverManagerMenu::FromSong(unset, false));
  EXPECT_FALSE(CoverManagerMenu::Contains(unset_items, CoverChoiceMenu::Action::Search));
  EXPECT_TRUE(CoverManagerMenu::Contains(unset_items, CoverChoiceMenu::Action::Clear));
  EXPECT_FALSE(CoverManagerMenu::Contains(unset_items, CoverChoiceMenu::Action::Unset));

  Song flagged;
  flagged.set_valid(true);
  const auto override_cover = CoverManagerMenu::FromSong(flagged, true, true);
  EXPECT_TRUE(override_cover.some_with_covers);
  EXPECT_TRUE(CoverManagerMenu::Contains(CoverManagerMenu::VisibleCoverItems(override_cover), CoverChoiceMenu::Action::Show));
}

TEST(AlbumCoverSearcher, PrefersRequestedSong) {
  Song requested;
  requested.set_valid(true);
  requested.set_artist("Portishead");
  requested.set_album("Dummy");
  Song fallback;
  fallback.set_valid(true);
  fallback.set_artist("Fleet Foxes");
  fallback.set_album("Helplessness Blues");
  const Song chosen = AlbumCoverSearcher::PreferredSong(requested, fallback);
  EXPECT_EQ("Portishead", chosen.artist());
  EXPECT_EQ("Dummy", chosen.album());
  const Song missing = AlbumCoverSearcher::PreferredSong(Song(), fallback);
  EXPECT_EQ("Fleet Foxes", missing.artist());
  EXPECT_EQ("Helplessness Blues", missing.album());
}

TEST(CoverManagerView, LabelsAndHideIndexMatchQt) {
  EXPECT_STREQ("View", CoverManagerView::ButtonLabel());
  EXPECT_STREQ("view-grid-symbolic", CoverManagerView::ButtonIcon());
  EXPECT_STREQ("Enter search terms here", CoverManagerView::SearchPlaceholder());
  EXPECT_EQ(3, CoverManagerView::kCount);
  EXPECT_STREQ("All albums", CoverManagerView::kLabels[0]);
  EXPECT_STREQ("Albums with covers", CoverManagerView::kLabels[1]);
  EXPECT_STREQ("Albums without covers", CoverManagerView::kLabels[2]);
  EXPECT_EQ(AlbumCoverManagerList::HideCovers::None, CoverManagerView::HideFromIndex(0));
  EXPECT_EQ(AlbumCoverManagerList::HideCovers::WithoutCovers, CoverManagerView::HideFromIndex(1));
  EXPECT_EQ(AlbumCoverManagerList::HideCovers::WithCovers, CoverManagerView::HideFromIndex(2));
  EXPECT_EQ(0, CoverManagerView::IndexFromHide(AlbumCoverManagerList::HideCovers::None));
  EXPECT_EQ(1, CoverManagerView::IndexFromHide(AlbumCoverManagerList::HideCovers::WithoutCovers));
  EXPECT_EQ(2, CoverManagerView::IndexFromHide(AlbumCoverManagerList::HideCovers::WithCovers));
  EXPECT_EQ(0, CoverManagerView::ClampIndex(-3));
  EXPECT_EQ(2, CoverManagerView::ClampIndex(9));
}

TEST(CoverManagerMenu, KeyboardUsesAlbumSelection) {
  EXPECT_TRUE(CoverManagerMenu::IsKeyboardTrigger(CoverManagerMenu::kMenu, 0));
  EXPECT_TRUE(CoverManagerMenu::IsKeyboardTrigger(CoverManagerMenu::kF10, CoverManagerMenu::kShiftMask));
  EXPECT_FALSE(CoverManagerMenu::IsKeyboardTrigger(CoverManagerMenu::kF10, 0));
  EXPECT_FALSE(CoverManagerMenu::IsKeyboardTrigger(ListBoxKeyboard::kReturn, 0));
  EXPECT_TRUE(CoverManagerMenu::ShouldShowMenu(true, true));
  EXPECT_FALSE(CoverManagerMenu::ShouldShowMenu(true, false));
  EXPECT_FALSE(CoverManagerMenu::ShouldShowMenu(false, true));
}

TEST(CoverManagerActivate, AlbumEnterShowsCoverNotPlaylist) {
  EXPECT_TRUE(CoverManagerActivate::IsEnter(ListBoxKeyboard::kReturn));
  EXPECT_TRUE(CoverManagerActivate::IsEnter(ListBoxKeyboard::kKPEnter));
  EXPECT_FALSE(CoverManagerActivate::IsEnter(ListBoxKeyboard::kEscape));
  EXPECT_EQ(CoverManagerActivate::Action::ShowCover, CoverManagerActivate::ForAlbumEnter());
  EXPECT_EQ(CoverManagerActivate::Action::None, CoverManagerActivate::ForArtistEnter());
  EXPECT_FALSE(CoverManagerActivate::AlbumEnterAddsToPlaylist());
}

TEST(CoverManagerActions, DoubleClickAndFetchFinishMatchQt) {
  EXPECT_STREQ("Cover Manager", CoverManagerActions::WindowTitle());
  EXPECT_TRUE(CoverManagerActions::DoubleClickShowsCover());
  EXPECT_FALSE(CoverManagerActions::ShowStatisticsWhenFetchFinishes(false, false, 3));
  EXPECT_FALSE(CoverManagerActions::ShowStatisticsWhenFetchFinishes(true, true, 3));
  EXPECT_FALSE(CoverManagerActions::ShowStatisticsWhenFetchFinishes(true, false, 0));
  EXPECT_TRUE(CoverManagerActions::ShowStatisticsWhenFetchFinishes(true, false, 2));
}

TEST(CoverManagerActions, CloseConfirmAndButtonsDuringFetch) {
  EXPECT_STREQ("Really cancel?", CoverManagerActions::CloseConfirmTitle());
  EXPECT_STREQ("Closing this window will stop searching for album covers.", CoverManagerActions::CloseConfirmMessage());
  EXPECT_STREQ("Abort", CoverManagerActions::CloseAbort());
  EXPECT_STREQ("Don't stop!", CoverManagerActions::CloseDontStop());
  EXPECT_FALSE(CoverManagerActions::ShouldConfirmCloseOnFetch(false));
  EXPECT_TRUE(CoverManagerActions::ShouldConfirmCloseOnFetch(true));
  EXPECT_TRUE(CoverManagerActions::FetchEnabled(false, true));
  EXPECT_FALSE(CoverManagerActions::FetchEnabled(true, true));
  EXPECT_FALSE(CoverManagerActions::FetchEnabled(false, false));
  EXPECT_FALSE(CoverManagerActions::FetchEnabled(true, false));
  EXPECT_TRUE(CoverManagerActions::ExportEnabled(false));
  EXPECT_FALSE(CoverManagerActions::ExportEnabled(true));
  EXPECT_TRUE(CoverManagerActions::CanCloseWithoutConfirm(false));
  EXPECT_FALSE(CoverManagerActions::CanCloseWithoutConfirm(true));
}

TEST(CoverManagerStats, TotalAndWithoutCoverMatchQt) {
  EXPECT_STREQ("Total albums:", CoverManagerStats::TotalLabel());
  EXPECT_STREQ("Without cover:", CoverManagerStats::WithoutLabel());
  EXPECT_STREQ("Fetch Missing Covers", CoverManagerStats::FetchMissing());
  EXPECT_STREQ("Export Covers", CoverManagerStats::Export());
  EXPECT_STREQ("Load", CoverManagerStats::Load());
  EXPECT_STREQ("Add to playlist", CoverManagerStats::AddToPlaylist());
  EXPECT_EQ(1, CoverManagerStats::WithoutCover(3, 2));
  EXPECT_EQ(0, CoverManagerStats::WithoutCover(2, 5));
  EXPECT_EQ(0, CoverManagerStats::WithoutCover(-1, 0));
  EXPECT_EQ("12", CoverManagerStats::CountText(12));
  EXPECT_EQ("0", CoverManagerStats::CountText(-4));
}

TEST(CoverProviderSettings, EnabledListMatchesQtSave) {
  EXPECT_STREQ("Cover providers", CoverProviderSettings::ProvidersGroup());
  EXPECT_STREQ("Choose the providers you want to use when searching for covers.", CoverProviderSettings::ProvidersHint());
  EXPECT_STREQ("Move up", CoverProviderSettings::MoveUp());
  EXPECT_STREQ("Move down", CoverProviderSettings::MoveDown());
  EXPECT_FALSE(CoverProviderSettings::EnabledFromStored(true, false, true, true));
  EXPECT_TRUE(CoverProviderSettings::EnabledFromStored(false, true, true, true));
  EXPECT_FALSE(CoverProviderSettings::EnabledFromStored(false, true, true, false));
  EXPECT_FALSE(CoverProviderSettings::EnabledFromStored(false, true, false, false, false));
  const std::vector<std::string> enabled =
      CoverProviderSettings::EnabledNames({{"Last.fm", true}, {"Discogs", false}, {"MusicBrainz", true}});
  ASSERT_EQ(2u, enabled.size());
  EXPECT_EQ("Last.fm", enabled.front());
  EXPECT_EQ("MusicBrainz", enabled.back());
}

TEST(CoverErrorMessage, MatchesQtWording) {
  EXPECT_EQ("Failed to open cover file /tmp/a.jpg for reading: Permission denied.",
            CoverErrorMessage::FailedToOpenForReading("/tmp/a.jpg", "Permission denied"));
  EXPECT_EQ("Cover file /tmp/empty.jpg is empty.", CoverErrorMessage::CoverFileEmpty("/tmp/empty.jpg"));
  EXPECT_EQ("Failed to open cover file /tmp/out.jpg for writing: Read-only file system.",
            CoverErrorMessage::FailedToOpenForWriting("/tmp/out.jpg", "Read-only file system"));
  EXPECT_EQ("Failed writing cover to file /tmp/out.jpg.", CoverErrorMessage::FailedWritingCover("/tmp/out.jpg"));
  EXPECT_EQ("Failed writing cover to file /tmp/out.jpg: No space left on device",
            CoverErrorMessage::FailedWritingCover("/tmp/out.jpg", "No space left on device"));
  EXPECT_EQ("Failed to delete cover file /tmp/cover.jpg: Permission denied.",
            CoverErrorMessage::FailedToDeleteCover("/tmp/cover.jpg", "Permission denied"));
  EXPECT_EQ("Could not save cover to file /tmp/album.flac.", CoverErrorMessage::CouldNotSaveCover("/tmp/album.flac"));
  EXPECT_TRUE(CoverErrorMessage::ShouldEmit("x"));
  EXPECT_FALSE(CoverErrorMessage::ShouldEmit({}));
}

TEST(UiError, ReportIgnoresEmptyAndForwardsMessages) {
  std::string seen;
  UiError::Bus().Clear();
  UiError::Bus().Connect([&seen](const std::string &message) { seen = message; });
  UiError::Report({});
  EXPECT_TRUE(seen.empty());
  UiError::Report("Could not save cover to file /tmp/album.flac.");
  EXPECT_EQ("Could not save cover to file /tmp/album.flac.", seen);
  UiError::Bus().Clear();
}

TEST(CoversSettingsLabels, MatchQtCoverPage) {
  EXPECT_STREQ("Cover providers", CoversSettingsLabels::ProvidersGroup());
  EXPECT_STREQ("Choose the providers you want to use when searching for covers.", CoversSettingsLabels::ProvidersHint());
  EXPECT_STREQ("Album cover types", CoversSettingsLabels::TypesGroup());
  EXPECT_STREQ("Saving album covers", CoversSettingsLabels::SavingGroup());
  EXPECT_STREQ("Filename:", CoversSettingsLabels::FilenameGroup());
  EXPECT_STREQ("Save album covers in album directory", CoversSettingsLabels::SaveAlbumDir());
  EXPECT_STREQ("Save album covers in cache directory", CoversSettingsLabels::SaveCache());
  EXPECT_STREQ("Save album covers as embedded cover", CoversSettingsLabels::SaveEmbedded());
  EXPECT_STREQ("Pattern", CoversSettingsLabels::FilenamePattern());
  EXPECT_STREQ("Random", CoversSettingsLabels::FilenameRandom());
  EXPECT_STREQ("Overwrite existing file", CoversSettingsLabels::Overwrite());
  EXPECT_STREQ("Lowercase filename", CoversSettingsLabels::Lowercase());
  EXPECT_STREQ("Replace spaces with dashes", CoversSettingsLabels::ReplaceSpaces());
  EXPECT_STREQ("1", CoversSettingsLabels::DefaultSaveType());
  EXPECT_STREQ("2", CoversSettingsLabels::DefaultFilename());
  ASSERT_EQ(3u, CoversSettingsLabels::SaveTypeChoices().size());
  EXPECT_EQ("2", CoversSettingsLabels::SaveTypeChoices().front().first);
  EXPECT_EQ("1", CoversSettingsLabels::SaveTypeChoices()[1].first);
  ASSERT_EQ(2u, CoversSettingsLabels::FilenameChoices().size());
  EXPECT_EQ("2", CoversSettingsLabels::FilenameChoices().front().first);
}
