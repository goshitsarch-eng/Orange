#include "streaming/streamingcover.h"
#include "streaming/streamingsearchitemdelegate.h"
#include "streaming/streamingsearchmodel.h"
#include "streaming/streamingsearchsortmodel.h"
#include "streaming/streamserviceplaylistitem.h"
#include "streaming/streamingdrag.h"
#include "streaming/streamsongmimedata.h"

#include <gtest/gtest.h>

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

TEST(StreamingDrag, JoinsSongUrls) {
  Song a(Song::Source::Tidal);
  a.set_url("tidal://track/1");
  Song b(Song::Source::Tidal);
  b.set_url("tidal://track/2");
  Song empty;
  EXPECT_EQ("tidal://track/1\ntidal://track/2", StreamingDrag::DragPayload({a, b, empty}));
  EXPECT_TRUE(StreamingDrag::DragPayload({}).empty());
}
