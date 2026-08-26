#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverexporter.h"
#include "covermanager/albumcoverfetcher.h"
#include "covermanager/albumcoverfetchersearch.h"
#include "covermanager/albumcoverloader.h"
#include "covermanager/albumcoverloaderoptions.h"
#include "covermanager/coverexportrunnable.h"
#include "covermanager/coversearchstatistics.h"
#include "covermanager/coversearchstatisticsdialog.h"
#include "covermanager/currentalbumcoverloader.h"
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
  EXPECT_NE(std::string::npos, text.find("Network requests: 3"));
  EXPECT_NE(std::string::npos, text.find("Chosen images: 2"));
  EXPECT_NE(std::string::npos, text.find("Last.fm"));
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

  FileUtils::Remove(FileUtils::Join(root, "cover.png"));
  FileUtils::Remove(source);
  FileUtils::Remove(song_path);
}

TEST(AlbumCoverFetcher, IncrementsRequestIds) {
  AlbumCoverFetcher fetcher(nullptr, nullptr);
  EXPECT_EQ(1u, fetcher.SearchForCovers("A", "B"));
  EXPECT_EQ(2u, fetcher.FetchAlbumCover("A", "B", "C", true));
  EXPECT_EQ(3u, fetcher.next_id());
}
