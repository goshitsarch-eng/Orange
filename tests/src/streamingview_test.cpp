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

TEST(StreamingDrag, JoinsSongUrls) {
  Song a(Song::Source::Tidal);
  a.set_url("tidal://track/1");
  Song b(Song::Source::Tidal);
  b.set_url("tidal://track/2");
  Song empty;
  EXPECT_EQ("tidal://track/1\ntidal://track/2", StreamingDrag::DragPayload({a, b, empty}));
  EXPECT_TRUE(StreamingDrag::DragPayload({}).empty());
}
