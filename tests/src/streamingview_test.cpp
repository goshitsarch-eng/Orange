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
