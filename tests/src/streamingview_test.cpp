#include "collection/collectionmodel.h"
#include "collection/collectiontree.h"
#include "streaming/streamingabort.h"
#include "streaming/streamingalbum.h"
#include "streaming/streamingcollectionactions.h"
#include "collection/collectionfilterfocus.h"
#include "collection/collectiontypeaheadscroll.h"
#include "streaming/streamingcollectionfilter.h"
#include "widgets/filtersearchkeyboard.h"
#include "widgets/listboxkeyboard.h"
#include "streaming/streamingcollectionlabels.h"
#include "streaming/streamingcollectionstore.h"
#include "streaming/streamingcollectiontree.h"
#include "streaming/streamingcoverdownload.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingsearchopts.h"
#include "streaming/streamingcover.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchgroup.h"
#include "streaming/streamingsearchhelp.h"
#include "streaming/streamingserviceenable.h"
#include "tidal/tidalservice.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamserviceplaylistitem.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamsongmimedata.h"
#include "core/database.h"
#include "core/settings.h"
#include "utilities/fileutils.h"

#include <ctime>
#include <gtest/gtest.h>
#include <unistd.h>

#include <set>
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

TEST(StreamingSearchHelp, IdleUntilFirstSearch) {
  EXPECT_STREQ("Enter search terms above to find music", StreamingSearchHelp::IdleText());
  EXPECT_STREQ("No results", StreamingSearchHelp::EmptyResultsText());
  EXPECT_STREQ(StreamingSearchHelp::IdleText(), StreamingSearchHelp::LabelFor(false));
  EXPECT_STREQ(StreamingSearchHelp::EmptyResultsText(), StreamingSearchHelp::LabelFor(true));
  EXPECT_STREQ("artists", StreamingSearchHelp::Artists());
  EXPECT_STREQ("albums", StreamingSearchHelp::Albums());
  EXPECT_STREQ("songs", StreamingSearchHelp::Songs());
}

TEST(StreamingCollectionLabels, MatchQtRefreshCatalogue) {
  EXPECT_STREQ("Refresh catalogue", StreamingCollectionLabels::Refresh());
  EXPECT_STREQ("Back", StreamingCollectionLabels::Back());
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
  EXPECT_TRUE(StreamingCover::LoadPrettyCovers(""));
  StreamingCover::SavePrettyCovers("Tidal", false);
  EXPECT_FALSE(StreamingCover::LoadPrettyCovers("Tidal"));
  StreamingCover::SavePrettyCovers("Tidal", true);
  EXPECT_TRUE(StreamingCover::LoadPrettyCovers("Tidal"));
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

  const auto grouped = StreamingSearchGroup::RowsFor({a, b, c}, StreamingSearchGroup::DefaultGrouping());
  EXPECT_EQ(4, StreamingSearchGroup::HeaderCount(grouped));
  EXPECT_EQ(3, StreamingSearchGroup::SongCount(grouped));
  const auto flat = StreamingSearchGroup::RowsFor({a, b, c}, {G::None, G::None, G::None});
  EXPECT_EQ(0, StreamingSearchGroup::HeaderCount(flat));
  EXPECT_EQ(3, StreamingSearchGroup::SongCount(flat));
  EXPECT_EQ("Roads", flat[0].song.title());
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
  std::string favorites_status;
  std::string favorites_failed;
  service.ArtistsUpdateStatus.Connect([&](const std::string &text) { artists_status = text; });
  service.ArtistsFailed.Connect([&](const std::string &text) { artists_failed = text; });
  service.FavoritesUpdateStatus.Connect([&](const std::string &text) { favorites_status = text; });
  service.FavoritesFailed.Connect([&](const std::string &text) { favorites_failed = text; });
  service.NotifyArtistsFailed("HTTP 401");
  service.NotifyFavoritesFailed("timeout");
  EXPECT_EQ("HTTP 401", artists_status);
  EXPECT_EQ("HTTP 401", artists_failed);
  EXPECT_EQ("timeout", favorites_status);
  EXPECT_EQ("timeout", favorites_failed);
}

TEST(StreamingService, ResetDropsInFlightCallbacks) {
  TidalService service(nullptr);
  int artists = 0;
  int albums = 0;
  int songs = 0;
  int searches = 0;
  int favorites = 0;
  auto artist_cb = service.GuardArtists([&](const SongList &) { ++artists; });
  auto album_cb = service.GuardAlbums([&](const SongList &) { ++albums; });
  auto song_cb = service.GuardSongs([&](const SongList &) { ++songs; });
  auto search_cb = service.GuardSearch([&](const SongList &) { ++searches; });
  auto favorite_cb = service.GuardFavorites([&](const SongList &) { ++favorites; });
  artist_cb({});
  album_cb({});
  song_cb({});
  search_cb({});
  favorite_cb({});
  EXPECT_EQ(1, artists);
  EXPECT_EQ(1, albums);
  EXPECT_EQ(1, songs);
  EXPECT_EQ(1, searches);
  EXPECT_EQ(1, favorites);
  service.ResetArtistsRequest();
  service.ResetAlbumsRequest();
  service.ResetSongsRequest();
  service.CancelSearch();
  service.ResetFavoritesRequest();
  artist_cb({});
  album_cb({});
  song_cb({});
  search_cb({});
  favorite_cb({});
  EXPECT_EQ(1, artists);
  EXPECT_EQ(1, albums);
  EXPECT_EQ(1, songs);
  EXPECT_EQ(1, searches);
  EXPECT_EQ(1, favorites);
  EXPECT_FALSE(service.ArtistsRequestCurrent(service.artists_generation() - 1));
  EXPECT_TRUE(service.ArtistsRequestCurrent(service.artists_generation()));
  EXPECT_FALSE(service.FavoritesRequestCurrent(service.favorites_generation() - 1));
  EXPECT_TRUE(service.FavoritesRequestCurrent(service.favorites_generation()));
}

TEST(StreamingService, FavoritesProgressUsesOwnChannel) {
  TidalService service(nullptr);
  std::string favorites_status;
  std::string songs_status;
  int favorites_max = 0;
  int favorites_value = -1;
  service.FavoritesUpdateStatus.Connect([&](const std::string &text) { favorites_status = text; });
  service.SongsUpdateStatus.Connect([&](const std::string &text) { songs_status = text; });
  service.FavoritesProgressSetMaximum.Connect([&](int maximum) { favorites_max = maximum; });
  service.FavoritesUpdateProgress.Connect([&](int value) { favorites_value = value; });
  service.StartFavoritesProgress(StreamingService::FavoriteType::Artists);
  EXPECT_EQ("Receiving artists...", favorites_status);
  EXPECT_TRUE(songs_status.empty());
  EXPECT_EQ(100, favorites_max);
  EXPECT_EQ(0, favorites_value);
  service.ReportFavoritesProgress(25, 50);
  EXPECT_EQ(50, favorites_value);
  service.StartFavoritesProgress(StreamingService::FavoriteType::Albums);
  EXPECT_EQ("Receiving albums...", favorites_status);
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
  EXPECT_EQ("Configure Tidal…", StreamingSearchOpts::ConfigureServiceLabel("Tidal"));
  EXPECT_EQ("Configure…", StreamingSearchOpts::ConfigureServiceLabel({}));
  EXPECT_STREQ("Search for this", StreamingSearchOpts::SearchForThisLabel());
  EXPECT_TRUE(StreamingSearchOpts::ShouldFocusOnShow());
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_albumartist("Portishead");
  EXPECT_EQ("Portishead", StreamingSearchOpts::QueryFromSong(song, T::Artists));
  EXPECT_EQ("Dummy", StreamingSearchOpts::QueryFromSong(song, T::Albums));
  EXPECT_EQ("Roads", StreamingSearchOpts::QueryFromSong(song, T::Songs));
  EXPECT_EQ("Portishead", StreamingSearchOpts::QueryFromPrimary("Portishead", song, T::Songs));
  EXPECT_EQ("Roads", StreamingSearchOpts::QueryFromPrimary({}, song, T::Songs));
  EXPECT_TRUE(StreamingSearchOpts::CanSearchForThis("Roads"));
  EXPECT_FALSE(StreamingSearchOpts::CanSearchForThis("   "));
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

TEST(StreamingSearchOpts, SearchTypePersistsLikeQt) {
  using T = StreamingService::SearchType;
  EXPECT_STREQ("type", StreamingSearchOpts::kSearchType);
  EXPECT_EQ(T::Artists, StreamingSearchOpts::DefaultSearchType());
  EXPECT_EQ(T::Artists, StreamingSearchOpts::ClampSearchType(0));
  EXPECT_EQ(T::Artists, StreamingSearchOpts::ClampSearchType(static_cast<int>(T::Artists)));
  EXPECT_EQ(T::Albums, StreamingSearchOpts::ClampSearchType(static_cast<int>(T::Albums)));
  EXPECT_EQ(T::Songs, StreamingSearchOpts::ClampSearchType(static_cast<int>(T::Songs)));
  EXPECT_EQ(T::Artists, StreamingSearchOpts::ClampSearchType(99));
  EXPECT_TRUE(StreamingSearchOpts::ShouldSaveOnActivate(true, false));
  EXPECT_FALSE(StreamingSearchOpts::ShouldSaveOnActivate(false, false));
  EXPECT_FALSE(StreamingSearchOpts::ShouldSaveOnActivate(true, true));
  EXPECT_TRUE(StreamingSearchOpts::ShouldReloadOnSettingsClose());
  EXPECT_EQ(T::Artists, StreamingSearchOpts::LoadSearchType({}));

  const char *group = "StreamingSearchTypeTest";
  Settings before;
  before.BeginGroup(group);
  const bool had = before.Contains(StreamingSearchOpts::kSearchType);
  const int old = before.IntValue(StreamingSearchOpts::kSearchType, static_cast<int>(StreamingSearchOpts::DefaultSearchType()));
  StreamingSearchOpts::SaveSearchType(group, T::Albums);
  EXPECT_EQ(T::Albums, StreamingSearchOpts::LoadSearchType(group));
  StreamingSearchOpts::SaveSearchType(group, T::Songs);
  EXPECT_EQ(T::Songs, StreamingSearchOpts::LoadSearchType(group));
  StreamingSearchOpts::SaveSearchType({}, T::Albums);
  EXPECT_EQ(T::Songs, StreamingSearchOpts::LoadSearchType(group));

  Settings restore;
  restore.BeginGroup(group);
  if (had) {
    restore.SetIntValue(StreamingSearchOpts::kSearchType, old);
  } else {
    restore.Remove(StreamingSearchOpts::kSearchType);
  }
  restore.Sync();
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

TEST(StreamingAlbum, EffectiveTitleAndApplyParent) {
  Song album(Song::Source::Qobuz);
  album.set_album("Dummy");
  album.set_title("Should ignore");
  album.set_album_id("88");
  album.set_artist("Portishead");
  album.set_albumartist("Portishead");
  album.set_artist_id("5");
  album.set_art_automatic("https://example.com/a.jpg");
  album.set_genre("Electronic");
  album.set_year(2008);
  EXPECT_EQ("Dummy", StreamingAlbum::EffectiveTitle(album));
  Song titled;
  titled.set_title("Only Title");
  EXPECT_EQ("Only Title", StreamingAlbum::EffectiveTitle(titled));

  Song empty(Song::Source::Qobuz);
  empty.set_title("Roads");
  SongList songs = {empty};
  StreamingAlbum::ApplyParent(songs, album);
  EXPECT_EQ("Dummy", songs[0].album());
  EXPECT_EQ("88", songs[0].album_id());
  EXPECT_EQ("Portishead", songs[0].artist());
  EXPECT_EQ("Portishead", songs[0].albumartist());
  EXPECT_EQ("5", songs[0].artist_id());
  EXPECT_EQ("https://example.com/a.jpg", songs[0].art_automatic());
  EXPECT_EQ("Electronic", songs[0].genre());
  EXPECT_EQ(2008, songs[0].year());

  Song kept(Song::Source::Qobuz);
  kept.set_album("Child");
  kept.set_artist("Child Artist");
  kept.set_year(1994);
  SongList existing = {kept};
  StreamingAlbum::ApplyParent(existing, album);
  EXPECT_EQ("Child", existing[0].album());
  EXPECT_EQ("Child Artist", existing[0].artist());
  EXPECT_EQ(1994, existing[0].year());
}

TEST(StreamingCollectionStore, TableNamesAndPersistRules) {
  EXPECT_EQ("tidal_artists_songs", StreamingCollectionStore::TableName("Tidal", StreamingCollectionStore::List::Artists));
  EXPECT_EQ("qobuz_albums_songs", StreamingCollectionStore::TableName("Qobuz", StreamingCollectionStore::List::Albums));
  EXPECT_EQ("spotify_songs", StreamingCollectionStore::TableName("Spotify", StreamingCollectionStore::List::Songs));
  EXPECT_EQ("subsonic_songs", StreamingCollectionStore::TableName("Subsonic", StreamingCollectionStore::List::Songs));
  EXPECT_TRUE(StreamingCollectionStore::TableName("Subsonic", StreamingCollectionStore::List::Artists).empty());
  EXPECT_TRUE(StreamingCollectionStore::TableName("Radio", StreamingCollectionStore::List::Songs).empty());
  EXPECT_TRUE(StreamingCollectionStore::ValidTable("tidal_songs"));
  EXPECT_FALSE(StreamingCollectionStore::ValidTable("songs; DROP TABLE songs"));
  EXPECT_FALSE(StreamingCollectionStore::ShouldPersist(true, true, {}));
  EXPECT_TRUE(StreamingCollectionStore::ShouldPersist(false, true, {}));
  EXPECT_TRUE(StreamingCollectionStore::ShouldKeepCache(true, {}));
  EXPECT_FALSE(StreamingCollectionStore::ShouldKeepCache(false, {}));
  Song song(Song::Source::Tidal);
  song.set_song_id("99");
  EXPECT_EQ("99", StreamingCollectionStore::PersistUrl(song));
  song.set_url("tidal://99");
  EXPECT_EQ("tidal://99", StreamingCollectionStore::PersistUrl(song));
  EXPECT_TRUE(StreamingCollectionStore::CanStore("Tidal", StreamingCollectionStore::List::Artists));
  EXPECT_FALSE(StreamingCollectionStore::CanStore("Subsonic", StreamingCollectionStore::List::Artists));
  EXPECT_TRUE(StreamingCollectionStore::CanStore("Subsonic", StreamingCollectionStore::List::Songs));
  EXPECT_EQ(3u, StreamingCollectionStore::AddableLists("Tidal").size());
  EXPECT_EQ(1u, StreamingCollectionStore::AddableLists("Subsonic").size());
  EXPECT_STREQ("Add to artists", StreamingCollectionStore::AddLabel(StreamingCollectionStore::List::Artists));
  EXPECT_STREQ("Added to songs", StreamingCollectionStore::AddedStatus(StreamingCollectionStore::List::Songs));
  EXPECT_STREQ("Removed from artists", StreamingCollectionStore::RemovedStatus(StreamingCollectionStore::List::Artists));
  StreamingCollectionStore::List list = StreamingCollectionStore::List::Songs;
  EXPECT_TRUE(StreamingCollectionStore::ListFromTab("artists", &list));
  EXPECT_EQ(StreamingCollectionStore::List::Artists, list);
  EXPECT_TRUE(StreamingCollectionStore::ListFromTab("albums", &list));
  EXPECT_EQ(StreamingCollectionStore::List::Albums, list);
  EXPECT_TRUE(StreamingCollectionStore::ListFromTab("songs", &list));
  EXPECT_EQ(StreamingCollectionStore::List::Songs, list);
  EXPECT_FALSE(StreamingCollectionStore::ListFromTab("favorites", &list));
  EXPECT_FALSE(StreamingCollectionStore::ListFromTab("search", &list));
  EXPECT_FALSE(StreamingCollectionStore::ListFromTab(nullptr, &list));
}

TEST(StreamingCollectionStore, ReplaceAndLoad) {
  const std::string path = "/tmp/strawberry-streaming-store-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  Song song(Song::Source::Tidal);
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_albumartist("Portishead");
  song.set_url("tidal://99");
  song.set_song_id("99");
  song.set_album_id("88");
  song.set_artist_id("5");
  song.set_art_automatic("https://example.com/a.jpg");
  song.set_year(1994);
  song.set_genre("Electronic");
  StreamingCollectionStore::Replace(&db, "tidal_songs", {song});
  const SongList loaded = StreamingCollectionStore::Load(&db, "tidal_songs");
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("Roads", loaded.front().title());
  EXPECT_EQ("Dummy", loaded.front().album());
  EXPECT_EQ("Portishead", loaded.front().artist());
  EXPECT_EQ("tidal://99", loaded.front().url());
  EXPECT_EQ("99", loaded.front().song_id());
  EXPECT_EQ("88", loaded.front().album_id());
  EXPECT_EQ("https://example.com/a.jpg", loaded.front().art_automatic());
  EXPECT_EQ(1994, loaded.front().year());
  EXPECT_EQ(Song::Source::Tidal, loaded.front().source());
  StreamingCollectionStore::Replace(&db, "tidal_songs", {});
  EXPECT_TRUE(StreamingCollectionStore::Load(&db, "tidal_songs").empty());
  EXPECT_TRUE(StreamingCollectionStore::Load(&db, "not_a_table").empty());
  unlink(path.c_str());
}

TEST(StreamingCollectionStore, MergeAppendsAndUpdatesByUrl) {
  const std::string path = "/tmp/strawberry-streaming-merge-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  Song first(Song::Source::Tidal);
  first.set_title("Roads");
  first.set_url("tidal://99");
  first.set_song_id("99");
  Song second(Song::Source::Tidal);
  second.set_title("Mysterons");
  second.set_url("tidal://100");
  second.set_song_id("100");
  StreamingCollectionStore::Replace(&db, "tidal_songs", {first});
  Song updated = first;
  updated.set_title("Roads (Remastered)");
  EXPECT_EQ(1, StreamingCollectionStore::Merge(&db, "tidal_songs", {updated, second}));
  const SongList loaded = StreamingCollectionStore::Load(&db, "tidal_songs");
  ASSERT_EQ(2u, loaded.size());
  bool saw_updated = false;
  bool saw_second = false;
  for (const Song &song : loaded) {
    if (song.song_id() == "99") {
      EXPECT_EQ("Roads (Remastered)", song.title());
      saw_updated = true;
    }
    if (song.song_id() == "100") {
      saw_second = true;
    }
  }
  EXPECT_TRUE(saw_updated);
  EXPECT_TRUE(saw_second);
  EXPECT_EQ(0, StreamingCollectionStore::Merge(&db, "tidal_songs", {}));
  EXPECT_EQ(0, StreamingCollectionStore::Merge(&db, "not_a_table", {second}));
  Song missing;
  missing.set_title("No key");
  const SongList merged = StreamingCollectionStore::MergeSongs({first}, {missing, second});
  EXPECT_EQ(2u, merged.size());
  EXPECT_EQ(1, StreamingCollectionStore::AddedCount({first}, merged));
  unlink(path.c_str());
}

TEST(StreamingCollectionStore, SubtractRemovesMatchingPersistKeys) {
  Song first(Song::Source::Tidal);
  first.set_title("Roads");
  first.set_url("tidal://99");
  Song second(Song::Source::Tidal);
  second.set_title("Mysterons");
  second.set_url("tidal://100");
  Song drop(Song::Source::Tidal);
  drop.set_url("tidal://99");
  const SongList remaining = StreamingCollectionStore::SubtractSongs({first, second}, {drop});
  ASSERT_EQ(1u, remaining.size());
  EXPECT_EQ("tidal://100", remaining.front().url());
  EXPECT_EQ(1, StreamingCollectionStore::RemovedCount({first, second}, remaining));
  EXPECT_EQ(0, StreamingCollectionStore::RemovedCount({first}, {first}));
  Song missing;
  missing.set_title("No key");
  EXPECT_EQ(2u, StreamingCollectionStore::SubtractSongs({first, second}, {missing}).size());

  const std::string path = "/tmp/strawberry-streaming-remove-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  StreamingCollectionStore::Replace(&db, "tidal_songs", {first, second});
  EXPECT_EQ(1, StreamingCollectionStore::Remove(&db, "tidal_songs", {drop}));
  const SongList loaded = StreamingCollectionStore::Load(&db, "tidal_songs");
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("tidal://100", loaded.front().url());
  EXPECT_EQ(0, StreamingCollectionStore::Remove(&db, "tidal_songs", {}));
  EXPECT_EQ(0, StreamingCollectionStore::Remove(&db, "not_a_table", {second}));
  unlink(path.c_str());
}

TEST(StreamingCollectionTree, CollapsedHidesSongsUntilExpanded) {
  Song a(Song::Source::Tidal);
  a.set_valid(true);
  a.set_title("Roads");
  a.set_artist("Portishead");
  a.set_albumartist("Portishead");
  a.set_album("Dummy");
  a.set_url("tidal://1");
  Song b(Song::Source::Tidal);
  b.set_valid(true);
  b.set_title("Mysterons");
  b.set_artist("Portishead");
  b.set_albumartist("Portishead");
  b.set_album("Dummy");
  b.set_url("tidal://2");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({a, b}, grouping, false, false, false);
  ASSERT_TRUE(model.root());
  std::set<std::string> expanded;
  EXPECT_FALSE(StreamingCollectionTree::FilterActive(""));
  EXPECT_TRUE(StreamingCollectionTree::FilterActive("port"));
  EXPECT_TRUE(StreamingCollectionTree::ShouldExpandAll(true));
  EXPECT_FALSE(StreamingCollectionTree::ShouldExpandAll(false));
  EXPECT_EQ(0, StreamingCollectionTree::VisibleSongCount(model.root(), false, expanded));
  EXPECT_EQ(2, StreamingCollectionTree::VisibleSongCount(model.root(), true, expanded));
  EXPECT_EQ(2, StreamingCollectionTree::SongsFromItem(model.root()).size());
  CollectionTree::CollectExpandableKeys(model.root(), &expanded);
  EXPECT_EQ(2, StreamingCollectionTree::VisibleSongCount(model.root(), false, expanded));
  EXPECT_GT(StreamingCollectionTree::VisibleRowCount(model.root(), false, expanded), 2);
  EXPECT_FALSE(StreamingCollectionTree::RepresentativeSong(model.root()).title().empty());
  EXPECT_EQ("2 items", StreamingCollectionTree::StatusText(2));
  EXPECT_EQ("1 item", StreamingCollectionTree::StatusText(1));
}

TEST(StreamingCollectionActions, KeyboardOpensContextMenu) {
  EXPECT_TRUE(StreamingCollectionActions::IsKeyboardTrigger(StreamingCollectionActions::kMenuKey, 0));
  EXPECT_TRUE(StreamingCollectionActions::IsKeyboardTrigger(StreamingCollectionActions::kF10Key, StreamingCollectionActions::kShiftMask));
  EXPECT_FALSE(StreamingCollectionActions::IsKeyboardTrigger(StreamingCollectionActions::kF10Key, 0));
  EXPECT_TRUE(StreamingCollectionActions::ShouldShowContextMenu(true));
  EXPECT_FALSE(StreamingCollectionActions::ShouldShowContextMenu(false));
}

TEST(StreamingCollectionActions, PlaylistActionsRequireSelection) {
  EXPECT_FALSE(StreamingCollectionActions::ShouldShowContextMenu(false));
  EXPECT_TRUE(StreamingCollectionActions::ShouldShowContextMenu(true));
  EXPECT_FALSE(StreamingCollectionActions::SelectionActionsEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::SelectionActionsEnabled(1));
  EXPECT_FALSE(StreamingCollectionActions::LoadEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::AppendEnabled(2));
  EXPECT_FALSE(StreamingCollectionActions::OpenInNewEnabled(0));
  EXPECT_FALSE(StreamingCollectionActions::EnqueueEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::EnqueueEnabled(1));
  EXPECT_TRUE(StreamingCollectionActions::EnqueueNextEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::EnqueueNextEnabled(3));
  EXPECT_TRUE(StreamingCollectionActions::EnqueueNextEnabled(0, StreamingCollectionActions::MenuContext::Collection));
  EXPECT_FALSE(StreamingCollectionActions::EnqueueNextEnabled(2, StreamingCollectionActions::MenuContext::Search));
  EXPECT_FALSE(StreamingCollectionActions::RemoveFromFavoritesEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::RemoveFromFavoritesEnabled(1, StreamingCollectionActions::MenuContext::Collection));
  EXPECT_FALSE(StreamingCollectionActions::RemoveFromFavoritesEnabled(1, StreamingCollectionActions::MenuContext::Search));
  EXPECT_TRUE(StreamingCollectionActions::SearchContextActionsEnabled(1));
  EXPECT_TRUE(StreamingCollectionActions::SearchContextActionsEnabled(2, StreamingCollectionActions::MenuContext::Search));
  EXPECT_FALSE(StreamingCollectionActions::SearchContextActionsEnabled(2, StreamingCollectionActions::MenuContext::Collection));
  EXPECT_FALSE(StreamingCollectionActions::SearchForThisEnabled(0));
  EXPECT_TRUE(StreamingCollectionActions::SearchForThisEnabled(1));
  EXPECT_FALSE(StreamingCollectionActions::SearchForThisEnabled(2));
  EXPECT_TRUE(StreamingCollectionActions::SearchForThisEnabled(1, StreamingCollectionActions::MenuContext::Search));
  EXPECT_FALSE(StreamingCollectionActions::SearchForThisEnabled(1, StreamingCollectionActions::MenuContext::Collection));
  EXPECT_FALSE(StreamingCollectionActions::FavoriteEnabled(2));
  EXPECT_FALSE(StreamingCollectionActions::FavoriteEnabled(2, StreamingCollectionActions::MenuContext::Collection));
  EXPECT_TRUE(StreamingCollectionActions::DisplayOptionsEnabled(StreamingCollectionActions::MenuContext::Collection));
  EXPECT_FALSE(StreamingCollectionActions::DisplayOptionsEnabled(StreamingCollectionActions::MenuContext::Search));
  EXPECT_STREQ("Display options", StreamingCollectionActions::DisplayOptionsLabel());
  EXPECT_TRUE(StreamingCollectionActions::HasDisplayOptionsTab("artists"));
  EXPECT_TRUE(StreamingCollectionActions::HasDisplayOptionsTab("favorites"));
  EXPECT_FALSE(StreamingCollectionActions::HasDisplayOptionsTab("search"));
  EXPECT_TRUE(StreamingCollectionActions::SearchSettingsEnabled(StreamingCollectionActions::MenuContext::Search));
  EXPECT_FALSE(StreamingCollectionActions::SearchSettingsEnabled(StreamingCollectionActions::MenuContext::Collection));
  EXPECT_STREQ("Group by", StreamingCollectionActions::SearchGroupByLabel());
  EXPECT_EQ(1u, StreamingCollectionActions::VisibleItems(0).size());
  EXPECT_EQ(StreamingCollectionActions::Action::EnqueueNext, StreamingCollectionActions::VisibleItems(0).front().id);
  EXPECT_TRUE(StreamingCollectionActions::VisibleItems(0, StreamingCollectionActions::MenuContext::Search).empty());
  EXPECT_EQ(StreamingCollectionActions::Items().size() - 1, StreamingCollectionActions::VisibleItems(2).size());
  EXPECT_FALSE(StreamingCollectionActions::Contains(StreamingCollectionActions::VisibleItems(2), StreamingCollectionActions::Action::Favorite));
  EXPECT_TRUE(StreamingCollectionActions::Contains(StreamingCollectionActions::VisibleItems(2), StreamingCollectionActions::Action::Unfavorite));
  EXPECT_FALSE(StreamingCollectionActions::Contains(StreamingCollectionActions::VisibleItems(2, StreamingCollectionActions::MenuContext::Search),
                                                    StreamingCollectionActions::Action::EnqueueNext));
  EXPECT_TRUE(StreamingCollectionActions::Contains(StreamingCollectionActions::VisibleItems(2, StreamingCollectionActions::MenuContext::Collection),
                                                   StreamingCollectionActions::Action::EnqueueNext));
}

TEST(StreamingCollectionFilter, AgeRatingUntaggedDuplicatesAndText) {
  Song tagged(Song::Source::Tidal);
  tagged.set_title("Roads");
  tagged.set_artist("Portishead");
  tagged.set_album("Dummy");
  tagged.set_ctime(static_cast<int>(std::time(nullptr)));
  tagged.set_rating(0.8f);
  tagged.set_url("tidal://track/1");
  Song old(Song::Source::Tidal);
  old.set_title("Wandering Star");
  old.set_artist("Portishead");
  old.set_album("Dummy");
  old.set_ctime(static_cast<int>(std::time(nullptr) - 10 * 86400));
  old.set_rating(0.2f);
  old.set_url("tidal://track/2");
  Song untagged(Song::Source::Tidal);
  untagged.set_title("Untitled");
  untagged.set_ctime(static_cast<int64_t>(std::time(nullptr)));
  untagged.set_url("tidal://track/3");
  Song dup(Song::Source::Tidal);
  dup.set_title("Roads");
  dup.set_artist("Portishead");
  dup.set_album("Dummy");
  dup.set_ctime(static_cast<int>(std::time(nullptr)));
  dup.set_rating(0.8f);
  dup.set_url("tidal://track/4");

  CollectionFilterOptions age;
  age.set_max_age(86400);
  EXPECT_EQ(3u, StreamingCollectionFilter::Apply({tagged, old, untagged, dup}, age, {}).size());

  CollectionFilterOptions rating;
  rating.set_min_rating(0.6f);
  EXPECT_EQ(2u, StreamingCollectionFilter::Apply({tagged, old, untagged, dup}, rating, {}).size());

  CollectionFilterOptions untagged_mode;
  untagged_mode.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  const SongList only_untagged = StreamingCollectionFilter::Apply({tagged, old, untagged, dup}, untagged_mode, "roads");
  ASSERT_EQ(1u, only_untagged.size());
  EXPECT_EQ("Untitled", only_untagged.front().title());
  EXPECT_FALSE(StreamingCollectionFilter::TextSearchEnabled(untagged_mode));

  CollectionFilterOptions duplicates;
  duplicates.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  EXPECT_EQ(2u, StreamingCollectionFilter::Apply({tagged, old, untagged, dup}, duplicates, {}).size());

  CollectionFilterOptions all;
  const SongList text = StreamingCollectionFilter::Apply({tagged, old, untagged, dup}, all, "wandering");
  ASSERT_EQ(1u, text.size());
  EXPECT_EQ("Wandering Star", text.front().title());
}

TEST(StreamingTypeAheadScroll, PositionsMatchAtTopLikeQt) {
  EXPECT_TRUE(CollectionTypeAheadScroll::ForceTop(true));
  EXPECT_FALSE(CollectionTypeAheadScroll::ForceTop(false));
  EXPECT_TRUE(CollectionTypeAheadScroll::ShouldScroll(0));
  EXPECT_FALSE(CollectionTypeAheadScroll::ShouldScroll(-1));
  EXPECT_DOUBLE_EQ(180.0, CollectionTypeAheadScroll::PositionAtTop(180.0, 0.0, 1200.0, 300.0));
}

TEST(StreamingFilterFocus, TreeKeysApplyLikeQtFocusOnFilter) {
  EXPECT_EQ(FilterSearchKeyboard::Action::FocusFilter, FilterSearchKeyboard::FromTreeKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(FilterSearchKeyboard::Action::FocusFilter, FilterSearchKeyboard::FromTreeKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(CollectionFilterFocus::Effect::Clear, CollectionFilterFocus::KeyEffect(ListBoxKeyboard::kEscape));
  EXPECT_EQ(CollectionFilterFocus::Effect::DeleteLast, CollectionFilterFocus::KeyEffect(ListBoxKeyboard::kBackSpace));
  EXPECT_TRUE(CollectionFilterFocus::Apply("portishead", CollectionFilterFocus::Effect::Clear).empty());
  EXPECT_EQ("qobu", CollectionFilterFocus::Apply("qobuz", CollectionFilterFocus::Effect::DeleteLast));
}

TEST(StreamingServiceEnable, HidesDisabledServicesLikeQt) {
  EXPECT_STREQ(TidalSettings::kSettingsGroup, StreamingServiceEnable::GroupFor("Tidal"));
  EXPECT_STREQ(SpotifySettings::kSettingsGroup, StreamingServiceEnable::GroupFor("Spotify"));
  EXPECT_STREQ(QobuzSettings::kSettingsGroup, StreamingServiceEnable::GroupFor("Qobuz"));
  EXPECT_STREQ(SubsonicSettings::kSettingsGroup, StreamingServiceEnable::GroupFor("Subsonic"));
  EXPECT_EQ(nullptr, StreamingServiceEnable::GroupFor("SomaFM"));
  EXPECT_FALSE(StreamingServiceEnable::DefaultEnabled("Tidal"));
  EXPECT_FALSE(StreamingServiceEnable::DefaultEnabled("Spotify"));
  EXPECT_FALSE(StreamingServiceEnable::DefaultEnabled("Qobuz"));
  EXPECT_FALSE(StreamingServiceEnable::DefaultEnabled("Subsonic"));
  EXPECT_TRUE(StreamingServiceEnable::DefaultEnabled("SomaFM"));
  EXPECT_TRUE(StreamingServiceEnable::ShouldList(true));
  EXPECT_FALSE(StreamingServiceEnable::ShouldList(false));
  EXPECT_TRUE(StreamingServiceEnable::ShouldShowStackPage(true));
  EXPECT_FALSE(StreamingServiceEnable::ShouldShowStackPage(false));
  EXPECT_TRUE(StreamingServiceEnable::ShouldRefreshOnSettingsClose());
  EXPECT_EQ("Tidal", StreamingServiceEnable::SelectVisible("Tidal", {"Tidal", "Qobuz"}));
  EXPECT_EQ("Tidal", StreamingServiceEnable::SelectVisible("Spotify", {"Tidal", "Qobuz"}));
  EXPECT_TRUE(StreamingServiceEnable::SelectVisible("Tidal", {}).empty());

  Settings settings;
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  const bool had_tidal = settings.Contains(TidalSettings::kEnabled);
  const bool old_tidal = settings.BoolValue(TidalSettings::kEnabled, TidalSettings::kDefaultEnabled);
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  const bool had_qobuz = settings.Contains(QobuzSettings::kEnabled);
  const bool old_qobuz = settings.BoolValue(QobuzSettings::kEnabled, QobuzSettings::kDefaultEnabled);
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  settings.SetBoolValue(TidalSettings::kEnabled, false);
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  settings.SetBoolValue(QobuzSettings::kEnabled, true);
  settings.Sync();
  EXPECT_FALSE(StreamingServiceEnable::IsEnabled("Tidal"));
  EXPECT_TRUE(StreamingServiceEnable::IsEnabled("Qobuz"));
  const std::vector<std::string> enabled = StreamingServiceEnable::EnabledAmong({"Tidal", "Qobuz", "SomaFM"});
  ASSERT_EQ(2u, enabled.size());
  EXPECT_EQ("Qobuz", enabled.front());
  EXPECT_EQ("SomaFM", enabled.back());
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  if (had_tidal) {
    settings.SetBoolValue(TidalSettings::kEnabled, old_tidal);
  } else {
    settings.Remove(TidalSettings::kEnabled);
  }
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  if (had_qobuz) {
    settings.SetBoolValue(QobuzSettings::kEnabled, old_qobuz);
  } else {
    settings.Remove(QobuzSettings::kEnabled);
  }
  settings.Sync();
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
