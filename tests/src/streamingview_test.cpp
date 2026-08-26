#include "streaming/streamingabort.h"
#include "streaming/streamingcoverdownload.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingsearchopts.h"
#include "streaming/streamingcover.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchgroup.h"
#include "tidal/tidalservice.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamserviceplaylistitem.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamsongmimedata.h"
#include "core/settings.h"
#include "utilities/fileutils.h"

#include <gtest/gtest.h>

#include <string>

TEST(StreamingSearchModel, FiltersAndSorts) {
  StreamingSearchModel model;
  Song a(Song::Source::Tidal);
  a.set_title("Roads");
  a.set_artist("Portishead");
  a.set_album("Dummy");
  Song b(Song::Source::Tidal);
  b.set_title("Helplessness Blues");
  b.set_artist("Fleet Foxes");
  b.set_album("Helplessness Blues");
  model.SetSongs({a, b});
  model.SetSort(StreamingSearchModel::SortField::Artist);
  SongList visible = model.Visible();
  ASSERT_EQ(2u, visible.size());
  EXPECT_EQ("Fleet Foxes", visible.front().artist());
  model.SetFilter("dummy");
  visible = model.Visible();
  ASSERT_EQ(1u, visible.size());
  EXPECT_EQ("Roads", visible.front().title());
  StreamingSearchSortModel sort(&model);
  model.SetFilter({});
  model.SetSort(StreamingSearchModel::SortField::Title);
  visible = sort.Visible();
  EXPECT_EQ("Helplessness Blues", visible.front().title());
}

TEST(StreamingCover, UrlCacheAndPrettyCovers) {
  EXPECT_EQ(32, StreamingCover::kArtHeight);
  EXPECT_TRUE(StreamingCover::kDefaultPrettyCovers);
  EXPECT_TRUE(StreamingCover::ShouldShowThumb(true));
  EXPECT_FALSE(StreamingCover::ShouldShowThumb(false));
  EXPECT_TRUE(StreamingCover::IsHttpUrl("https://resources.tidal.com/images/abc/1280x1280.jpg"));
  EXPECT_TRUE(StreamingCover::CanLoad("http://example.com/cover.jpg"));
  EXPECT_FALSE(StreamingCover::CanLoad("tidal://track/1"));
  EXPECT_FALSE(StreamingCover::CanLoad(""));

  Song song;
  song.set_url("tidal://track/1");
  song.set_art_automatic("https://resources.tidal.com/images/auto.jpg");
  EXPECT_EQ("https://resources.tidal.com/images/auto.jpg", StreamingCover::CoverUrl(song));
  EXPECT_TRUE(StreamingCover::CanLoad(song));
  EXPECT_EQ("https://resources.tidal.com/images/auto.jpg", StreamingCover::CacheKey(song));
  song.set_art_manual("https://cdn.example.com/manual.png");
  EXPECT_EQ("https://cdn.example.com/manual.png", StreamingCover::CoverUrl(song));
  EXPECT_EQ("https://cdn.example.com/manual.png", StreamingCover::CacheKey(song));
  Song no_art;
  no_art.set_url("qobuz://track/9");
  EXPECT_TRUE(StreamingCover::CoverUrl(no_art).empty());
  EXPECT_FALSE(StreamingCover::CanLoad(no_art));
  EXPECT_EQ("qobuz://track/9", StreamingCover::CacheKey(no_art));
}

TEST(StreamingSearchGroup, DefaultFlattenAndHeaders) {
  using G = CollectionGrouping::GroupBy;
  EXPECT_EQ(G::AlbumArtist, StreamingSearchGroup::DefaultGrouping().first);
  EXPECT_EQ(G::AlbumDisc, StreamingSearchGroup::DefaultGrouping().second);
  EXPECT_EQ(G::None, StreamingSearchGroup::DefaultGrouping().third);
  EXPECT_TRUE(StreamingSearchGroup::HasLevels(StreamingSearchGroup::DefaultGrouping()));
  EXPECT_FALSE(StreamingSearchGroup::HasLevels({G::None, G::None, G::None}));
  EXPECT_EQ(G::AlbumArtist, StreamingSearchGroup::FromSaved(0, 0, 0, false).first);
  EXPECT_EQ(G::Artist, StreamingSearchGroup::FromSaved(2, 3, 0, true).first);
  EXPECT_EQ(G::Album, StreamingSearchGroup::FromSaved(2, 3, 0, true).second);
  EXPECT_EQ(8, StreamingSearchGroup::IndentPixels(0));
  EXPECT_EQ(24, StreamingSearchGroup::IndentPixels(1));

  Song a(Song::Source::Tidal);
  a.set_albumartist("Portishead");
  a.set_artist("Portishead");
  a.set_album("Dummy");
  a.set_title("Roads");
  a.set_track(3);
  Song b(Song::Source::Tidal);
  b.set_albumartist("Portishead");
  b.set_artist("Portishead");
  b.set_album("Dummy");
  b.set_title("Mysterons");
  b.set_track(1);
  Song c(Song::Source::Tidal);
  c.set_albumartist("Fleet Foxes");
  c.set_artist("Fleet Foxes");
  c.set_album("Helplessness Blues");
  c.set_title("Montezuma");
  const CollectionGrouping::Node tree =
      CollectionGrouping::BuildTree({a, b, c}, StreamingSearchGroup::DefaultGrouping(), false, true, false);
  const auto rows = StreamingSearchGroup::Flatten(tree);
  EXPECT_EQ(4, StreamingSearchGroup::HeaderCount(rows));
  ASSERT_GE(rows.size(), 7u);
  EXPECT_TRUE(rows[0].header);
  EXPECT_EQ("Fleet Foxes", rows[0].label);
  EXPECT_TRUE(rows[1].header);
  EXPECT_EQ("Helplessness Blues", rows[1].label);
  EXPECT_FALSE(rows[2].header);
  EXPECT_EQ("Montezuma", rows[2].song.title());
  EXPECT_EQ("Portishead", rows[3].label);
  EXPECT_EQ("Dummy", rows[4].label);
  EXPECT_EQ("Mysterons", rows[5].song.title());
  EXPECT_EQ("Roads", rows[6].song.title());
}

TEST(StreamingSearchItemDelegate, PrimaryAndSecondary) {
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  EXPECT_EQ("Roads", StreamingSearchItemDelegate::PrimaryText(song));
  EXPECT_EQ("Portishead · Dummy", StreamingSearchItemDelegate::SecondaryText(song));
  StreamServicePlaylistItem item(song);
  EXPECT_EQ("Portishead - Roads", item.DisplayText());
  StreamSongMimeData mime;
  mime.songs.push_back(song);
  mime.source = Song::Source::Tidal;
  mime.service_name = "Tidal";
  EXPECT_EQ(1u, mime.songs.size());
  EXPECT_EQ(Song::Source::Tidal, mime.source);
}

TEST(StreamingProgress, QueryProgressAndStatus) {
  EXPECT_TRUE(StreamingProgress::HasQuery(" roads "));
  EXPECT_FALSE(StreamingProgress::HasQuery({}));
  EXPECT_FALSE(StreamingProgress::HasQuery(" \t"));
  EXPECT_TRUE(StreamingProgress::ShouldShow(true, "roads"));
  EXPECT_FALSE(StreamingProgress::ShouldShow(false, "roads"));
  EXPECT_FALSE(StreamingProgress::ShouldShow(true, "  "));
  EXPECT_EQ(0, StreamingProgress::GetProgress(0, 0));
  EXPECT_EQ(0, StreamingProgress::GetProgress(1, 0));
  EXPECT_EQ(50, StreamingProgress::GetProgress(1, 2));
  EXPECT_EQ(100, StreamingProgress::GetProgress(2, 2));
  EXPECT_DOUBLE_EQ(0.0, StreamingProgress::Fraction(0, 100));
  EXPECT_DOUBLE_EQ(0.0, StreamingProgress::Fraction(10, 0));
  EXPECT_DOUBLE_EQ(0.5, StreamingProgress::Fraction(50, 100));
  EXPECT_DOUBLE_EQ(1.0, StreamingProgress::Fraction(100, 100));
  EXPECT_STREQ("Searching...", StreamingProgress::Searching());
  EXPECT_STREQ("Receiving artists...", StreamingProgress::ReceivingArtists());
  EXPECT_STREQ("Receiving albums...", StreamingProgress::ReceivingAlbums());
  EXPECT_STREQ("Receiving songs...", StreamingProgress::ReceivingSongs());
  EXPECT_STREQ("Retrieving albums...", StreamingProgress::RetrievingAlbums());
  EXPECT_EQ("Receiving albums for 1 artist...", StreamingProgress::ReceivingAlbumsForArtists(1));
  EXPECT_EQ("Receiving albums for 3 artists...", StreamingProgress::ReceivingAlbumsForArtists(3));
  EXPECT_EQ("Receiving songs for 1 album...", StreamingProgress::ReceivingSongsForAlbums(1));
  EXPECT_EQ("Receiving songs for 4 albums...", StreamingProgress::ReceivingSongsForAlbums(4));
  EXPECT_EQ("Receiving album cover for 1 album...", StreamingProgress::ReceivingCovers(1));
  EXPECT_EQ("Receiving album covers for 2 albums...", StreamingProgress::ReceivingCovers(2));
  EXPECT_EQ("Retrieving songs for 1 album...", StreamingProgress::RetrievingSongsForAlbums(1));
  EXPECT_EQ("Retrieving album covers for 2 albums...", StreamingProgress::RetrievingCovers(2));
  EXPECT_TRUE(StreamingProgress::ShouldShowBrowse(true, true));
  EXPECT_FALSE(StreamingProgress::ShouldShowBrowse(false, true));
  EXPECT_FALSE(StreamingProgress::ShouldShowBrowse(true, false));
}

TEST(StreamingService, StartSearchProgressEmitsSearching) {
  TidalService service(nullptr);
  std::string status;
  int max = 0;
  int value = -1;
  int id = 0;
  service.SearchUpdateStatus.Connect([&](int search_id, const std::string &text) {
    id = search_id;
    status = text;
  });
  service.SearchProgressSetMaximum.Connect([&](int, int maximum) { max = maximum; });
  service.SearchUpdateProgress.Connect([&](int, int progress) { value = progress; });
  EXPECT_EQ(0, service.last_search_id());
  EXPECT_EQ(1, service.StartSearchProgress());
  EXPECT_EQ(1, service.last_search_id());
  EXPECT_EQ(1, id);
  EXPECT_EQ("Searching...", status);
  EXPECT_EQ(StreamingProgress::kDefaultMaximum, max);
  EXPECT_EQ(0, value);
  EXPECT_EQ(2, service.StartSearchProgress());
}

TEST(StreamingService, BrowseProgressEmitsReceiving) {
  TidalService service(nullptr);
  EXPECT_TRUE(service.show_progress());
  std::string artists;
  std::string albums;
  std::string songs;
  int artists_max = 0;
  int albums_value = -1;
  service.ArtistsUpdateStatus.Connect([&](const std::string &text) { artists = text; });
  service.ArtistsProgressSetMaximum.Connect([&](int maximum) { artists_max = maximum; });
  service.AlbumsUpdateStatus.Connect([&](const std::string &text) { albums = text; });
  service.AlbumsUpdateProgress.Connect([&](int value) { albums_value = value; });
  service.SongsUpdateStatus.Connect([&](const std::string &text) { songs = text; });
  service.StartArtistsProgress();
  service.StartAlbumsProgress();
  service.StartSongsProgress();
  EXPECT_EQ("Receiving artists...", artists);
  EXPECT_EQ(StreamingProgress::kDefaultMaximum, artists_max);
  EXPECT_EQ("Receiving albums...", albums);
  EXPECT_EQ(0, albums_value);
  EXPECT_EQ("Receiving songs...", songs);
}

TEST(StreamingAbort, VisibilityAndGeneration) {
  EXPECT_STREQ("Abort", StreamingAbort::AbortLabel());
  EXPECT_STREQ("Close", StreamingAbort::CloseLabel());
  EXPECT_TRUE(StreamingAbort::ShouldShowAbort(true));
  EXPECT_FALSE(StreamingAbort::ShouldShowAbort(false));
  EXPECT_TRUE(StreamingAbort::ShouldShowClose(false, true));
  EXPECT_FALSE(StreamingAbort::ShouldShowClose(true, true));
  EXPECT_FALSE(StreamingAbort::ShouldShowClose(false, false));
  EXPECT_TRUE(StreamingAbort::ShouldShowAction(true, false));
  EXPECT_TRUE(StreamingAbort::ShouldShowAction(false, true));
  EXPECT_FALSE(StreamingAbort::ShouldShowAction(false, false));
  EXPECT_STREQ("Abort", StreamingAbort::ButtonLabel(true, false));
  EXPECT_STREQ("Close", StreamingAbort::ButtonLabel(false, true));
  EXPECT_EQ("HTTP 401", StreamingAbort::HttpError(401, {}));
  EXPECT_EQ("timeout", StreamingAbort::HttpError(0, "timeout"));
  EXPECT_EQ("Network error", StreamingAbort::HttpError(0, {}));
  EXPECT_EQ(1, StreamingAbort::NextGeneration(0));
  EXPECT_TRUE(StreamingAbort::IsCurrent(2, 2));
  EXPECT_FALSE(StreamingAbort::IsCurrent(1, 2));
}

TEST(StreamingService, NotifyFailedEmitsStatusAndFailed) {
  TidalService service(nullptr);
  std::string artists_status;
  std::string artists_failed;
  std::string favorites_failed;
  service.ArtistsUpdateStatus.Connect([&](const std::string &text) { artists_status = text; });
  service.ArtistsFailed.Connect([&](const std::string &text) { artists_failed = text; });
  service.FavoritesFailed.Connect([&](const std::string &text) { favorites_failed = text; });
  service.NotifyArtistsFailed("HTTP 401");
  service.NotifyFavoritesFailed("timeout");
  EXPECT_EQ("HTTP 401", artists_status);
  EXPECT_EQ("HTTP 401", artists_failed);
  EXPECT_EQ("timeout", favorites_failed);
}

TEST(StreamingService, ResetDropsInFlightCallbacks) {
  TidalService service(nullptr);
  int artists = 0;
  int albums = 0;
  int songs = 0;
  int searches = 0;
  auto artist_cb = service.GuardArtists([&](const SongList &) { ++artists; });
  auto album_cb = service.GuardAlbums([&](const SongList &) { ++albums; });
  auto song_cb = service.GuardSongs([&](const SongList &) { ++songs; });
  auto search_cb = service.GuardSearch([&](const SongList &) { ++searches; });
  artist_cb({});
  album_cb({});
  song_cb({});
  search_cb({});
  EXPECT_EQ(1, artists);
  EXPECT_EQ(1, albums);
  EXPECT_EQ(1, songs);
  EXPECT_EQ(1, searches);
  service.ResetArtistsRequest();
  service.ResetAlbumsRequest();
  service.ResetSongsRequest();
  service.CancelSearch();
  artist_cb({});
  album_cb({});
  song_cb({});
  search_cb({});
  EXPECT_EQ(1, artists);
  EXPECT_EQ(1, albums);
  EXPECT_EQ(1, songs);
  EXPECT_EQ(1, searches);
  EXPECT_FALSE(service.ArtistsRequestCurrent(service.artists_generation() - 1));
  EXPECT_TRUE(service.ArtistsRequestCurrent(service.artists_generation()));
}

TEST(StreamingSearchOpts, FetchAlbumsRemasteredAndExplicit) {
  using T = StreamingService::SearchType;
  EXPECT_TRUE(StreamingSearchOpts::ShouldFetchAlbums(true, T::Songs));
  EXPECT_FALSE(StreamingSearchOpts::ShouldFetchAlbums(true, T::Artists));
  EXPECT_FALSE(StreamingSearchOpts::ShouldFetchAlbums(false, T::Songs));
  EXPECT_FALSE(StreamingSearchOpts::FetchAlbumsEnabled("Subsonic"));
  EXPECT_FALSE(StreamingSearchOpts::AppendExplicitEnabled("Qobuz"));

  Song remastered;
  remastered.set_title("Roads (Remastered)");
  remastered.set_album("Dummy [Remastered]");
  remastered.set_album_id("8");
  remastered.set_song_id("99");
  Song explicit_album;
  explicit_album.set_title("Dummy");
  explicit_album.set_album("Dummy");
  explicit_album.set_album_id("8");
  explicit_album.set_comment("explicit");
  const SongList cleaned = StreamingSearchOpts::Finish({remastered, explicit_album}, true, true);
  ASSERT_EQ(2u, cleaned.size());
  EXPECT_EQ("Roads", cleaned[0].title());
  EXPECT_EQ("Dummy", cleaned[0].album());
  EXPECT_EQ("Dummy (Explicit)", cleaned[1].album());
  EXPECT_EQ("Dummy (Explicit)", cleaned[1].title());
  EXPECT_TRUE(StreamingSearchOpts::LooksExplicit(explicit_album));

  Song a;
  a.set_album_id("8");
  Song b;
  b.set_album_id("8");
  Song c;
  c.set_album_id("9");
  Song none;
  EXPECT_EQ(std::vector<std::string>({"8", "9"}), StreamingSearchOpts::UniqueAlbumIds({a, b, c, none}));
}

TEST(StreamingSearchOpts, DelayLimitsAndConfigure) {
  using T = StreamingService::SearchType;
  EXPECT_STREQ("Configure…", StreamingSearchOpts::ConfigureLabel());
  EXPECT_EQ(1500, StreamingSearchOpts::kDefaultDelayMs);
  EXPECT_EQ(4, StreamingSearchOpts::DefaultLimitFor(T::Artists));
  EXPECT_EQ(10, StreamingSearchOpts::DefaultLimitFor(T::Albums));
  EXPECT_EQ(10, StreamingSearchOpts::DefaultLimitFor(T::Songs));
  EXPECT_STREQ("artistssearchlimit", StreamingSearchOpts::LimitKey(T::Artists));
  EXPECT_STREQ("albumssearchlimit", StreamingSearchOpts::LimitKey(T::Albums));
  EXPECT_STREQ("songssearchlimit", StreamingSearchOpts::LimitKey(T::Songs));
  EXPECT_TRUE(StreamingSearchOpts::HasSearchLimits("Tidal"));
  EXPECT_TRUE(StreamingSearchOpts::HasSearchLimits("Qobuz"));
  EXPECT_TRUE(StreamingSearchOpts::HasSearchLimits("Spotify"));
  EXPECT_FALSE(StreamingSearchOpts::HasSearchLimits("Subsonic"));
  EXPECT_EQ(0, StreamingSearchOpts::ClampDelay(-20));
  EXPECT_EQ(1500, StreamingSearchOpts::ClampDelay(1500));
  EXPECT_EQ(4, StreamingSearchOpts::ClampLimit(0, 4));
  EXPECT_EQ(8, StreamingSearchOpts::ClampLimit(8, 4));
  EXPECT_TRUE(StreamingSearchOpts::ShouldDelay(1500, false));
  EXPECT_FALSE(StreamingSearchOpts::ShouldDelay(1500, true));
  EXPECT_FALSE(StreamingSearchOpts::ShouldDelay(0, false));
  EXPECT_TRUE(StreamingSearchOpts::ShouldSearchOnChange("ab"));
  EXPECT_FALSE(StreamingSearchOpts::ShouldSearchOnChange("a"));
  EXPECT_EQ(-1, StreamingPage::Remaining(0, 10));
  EXPECT_EQ(6, StreamingPage::Remaining(10, 4));
  EXPECT_EQ(50, StreamingPage::PageLimit(50, 0, 10));
  EXPECT_EQ(6, StreamingPage::PageLimit(50, 10, 4));
  EXPECT_TRUE(StreamingPage::ReachedMax(10, 10));
  EXPECT_FALSE(StreamingPage::ReachedMax(9, 10));
  EXPECT_FALSE(StreamingPage::ReachedMax(10, 0));
}

TEST(StreamingCoverDownload, HelpersAndSettings) {
  EXPECT_TRUE(StreamingCoverDownload::HasDownloadSetting("Tidal"));
  EXPECT_TRUE(StreamingCoverDownload::HasDownloadSetting("Qobuz"));
  EXPECT_TRUE(StreamingCoverDownload::HasDownloadSetting("Spotify"));
  EXPECT_TRUE(StreamingCoverDownload::HasDownloadSetting("Subsonic"));
  EXPECT_FALSE(StreamingCoverDownload::HasDownloadSetting("Radio"));
  EXPECT_STREQ("Tidal", StreamingCoverDownload::SourceGroup(Song::Source::Tidal));
  EXPECT_EQ("1280x1280.jpg", StreamingCoverDownload::FileNameFromUrl("https://resources.tidal.com/images/aa/1280x1280.jpg?token=1"));
  EXPECT_EQ("42-cover.jpg", StreamingCoverDownload::CacheFilename("Tidal", "42", "https://example.com/cover.jpg"));
  EXPECT_EQ("42", StreamingCoverDownload::CacheFilename("Spotify", "42", "https://example.com/img.png"));
  EXPECT_TRUE(StreamingCoverDownload::CacheFilename("Tidal", "", "https://example.com/a.jpg").empty());
  EXPECT_EQ("Receiving album cover for 1 album...", StreamingCoverDownload::Receiving(1));
  EXPECT_EQ("Receiving album covers for 3 albums...", StreamingCoverDownload::Receiving(3));
  EXPECT_TRUE(StreamingCoverDownload::IsCoverArtId("al-12"));
  EXPECT_FALSE(StreamingCoverDownload::IsCoverArtId("https://example.com/a.jpg"));
  EXPECT_FALSE(StreamingCoverDownload::IsCoverArtId("/tmp/a.jpg"));
  EXPECT_TRUE(StreamingCover::IsLocalUrl("file:///tmp/a.jpg"));
  EXPECT_TRUE(StreamingCover::IsLocalUrl("/tmp/a.jpg"));
  EXPECT_TRUE(StreamingCover::CanLoad("/tmp/a.jpg"));
  EXPECT_FALSE(StreamingCover::IsLocalUrl("https://example.com/a.jpg"));
  EXPECT_TRUE(StreamingCover::ValidTidalCoverSize("640x640"));
  EXPECT_FALSE(StreamingCover::ValidTidalCoverSize("99x99"));
  EXPECT_EQ("640x640", StreamingCover::ClampTidalCoverSize("nope"));
  EXPECT_EQ("https://resources.tidal.com/images/aa/640x640.jpg",
            StreamingCover::WithTidalCoverSize("https://resources.tidal.com/images/aa/1280x1280.jpg", "640x640"));
  Song sized(Song::Source::Tidal);
  sized.set_art_automatic("https://resources.tidal.com/images/aa/1280x1280.jpg");
  SongList sized_songs = {sized};
  StreamingCover::ApplyTidalCoverSize(sized_songs, "320x320");
  EXPECT_EQ("https://resources.tidal.com/images/aa/320x320.jpg", sized_songs[0].art_automatic());
  Song song(Song::Source::Tidal);
  song.set_album_id("42");
  song.set_art_automatic("https://example.com/a.jpg");
  EXPECT_TRUE(StreamingCoverDownload::NeedsDownload(song));
  EXPECT_EQ(1u, StreamingCoverDownload::UniqueAlbums({song}).size());
  EXPECT_TRUE(StreamingCoverDownload::ShouldDownload(true, {song}));
  EXPECT_FALSE(StreamingCoverDownload::ShouldDownload(false, {song}));
  song.set_art_automatic("/tmp/a.jpg");
  EXPECT_FALSE(StreamingCoverDownload::NeedsDownload(song));
  SongList songs = {song};
  songs[0].set_art_automatic("https://example.com/a.jpg");
  StreamingCoverDownload::ApplyLocalCover(songs, "42", "/tmp/cover.jpg");
  EXPECT_EQ(FileUtils::UriFromPath("/tmp/cover.jpg"), songs[0].art_automatic());
  Song sub(Song::Source::Subsonic);
  sub.set_art_automatic("al-9");
  SongList sub_songs = {sub};
  StreamingCoverDownload::ApplyCoverArtIds(sub_songs, [](const std::string &id) { return "https://server/getCoverArt?id=" + id; });
  EXPECT_EQ("https://server/getCoverArt?id=al-9", sub_songs[0].art_automatic());
  Settings settings;
  settings.BeginGroup("Tidal");
  settings.SetBoolValue(StreamingCoverDownload::kDownloadAlbumCovers, false);
  settings.Sync();
  EXPECT_FALSE(StreamingCoverDownload::Enabled("Tidal"));
  settings.SetBoolValue(StreamingCoverDownload::kDownloadAlbumCovers, true);
  settings.Sync();
  EXPECT_TRUE(StreamingCoverDownload::Enabled("Tidal"));
}

TEST(StreamingDrag, JoinsSongUrls) {
  Song a(Song::Source::Tidal);
  a.set_url("tidal://track/1");
  Song b(Song::Source::Tidal);
  b.set_url("tidal://track/2");
  Song empty;
  EXPECT_EQ("tidal://track/1\ntidal://track/2", StreamingDrag::DragPayload({a, b, empty}));
  EXPECT_TRUE(StreamingDrag::DragPayload({}).empty());
}
