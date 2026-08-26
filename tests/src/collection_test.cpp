#include "collection/collectionbackend.h"
#include "collection/collectiondirectorymodel.h"
#include "collection/collectionfilter.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectionitem.h"
#include "collection/collectionitemdelegate.h"
#include "collection/collectionmodel.h"
#include "collection/collectionplaylistitem.h"
#include "collection/collectionquery.h"
#include "collection/collectiontask.h"
#include "core/database.h"
#include "core/taskmanager.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <ctime>
#include <string>

namespace {

Song MakeSong(const std::string &title, const std::string &artist, const std::string &album, int64_t ctime = -1) {
  Song song(Song::Source::Collection);
  song.set_title(title);
  song.set_artist(artist);
  song.set_album(album);
  song.set_albumartist(artist);
  song.set_url("file:///tmp/music/" + title + ".flac");
  song.set_length_nanosec(180000000000);
  song.set_bitrate(320);
  song.set_samplerate(44100);
  song.set_bitdepth(16);
  song.set_ctime(ctime);
  song.set_valid(true);
  return song;
}

}  // namespace

TEST(CollectionFilterOptions, MatchesAgeRatingAndText) {
  CollectionFilterOptions options;
  Song recent = MakeSong("Roads", "Portishead", "Dummy", std::time(nullptr));
  Song old = MakeSong("Wandering Star", "Portishead", "Dummy", std::time(nullptr) - 10 * 86400);
  old.set_url("file:///tmp/music/wandering.flac");
  options.set_max_age(86400);
  EXPECT_TRUE(options.Matches(recent));
  EXPECT_FALSE(options.Matches(old));

  CollectionFilterOptions rating;
  recent.set_rating(0.8f);
  old.set_rating(0.2f);
  rating.set_min_rating(0.6f);
  EXPECT_TRUE(rating.Matches(recent));
  EXPECT_FALSE(rating.Matches(old));

  CollectionFilterOptions text;
  text.set_filter_text("dummy");
  EXPECT_TRUE(text.Matches(recent));
  Song other = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  EXPECT_FALSE(text.Matches(other));
  EXPECT_EQ(CollectionFilterOptions::FilterMode::All, text.filter_mode());
}

TEST(CollectionFilterOptions, FilterModeClearsText) {
  CollectionFilterOptions options;
  options.set_filter_text("roads");
  EXPECT_TRUE(options.has_filter_text());
  options.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  EXPECT_FALSE(options.has_filter_text());
  EXPECT_TRUE(options.filter_text().empty());
}

TEST(CollectionQuery, BuildsSqlForAgeUntaggedDuplicatesAndUnavailable) {
  CollectionFilterOptions age;
  age.set_max_age(86400);
  CollectionQuery aged(nullptr, "songs", age);
  aged.SetColumnSpec("ROWID, title");
  const std::string age_sql = aged.Sql();
  EXPECT_NE(std::string::npos, age_sql.find("ctime > "));
  EXPECT_NE(std::string::npos, age_sql.find("unavailable = 0"));

  CollectionFilterOptions untagged;
  untagged.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  CollectionQuery untagged_query(nullptr, "songs", untagged);
  EXPECT_NE(std::string::npos, untagged_query.Sql().find("(artist = '' OR album = '' OR title = '')"));

  CollectionFilterOptions duplicates;
  duplicates.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  CollectionQuery dup(nullptr, "songs", duplicates);
  const std::string dup_sql = dup.Sql();
  EXPECT_NE(std::string::npos, dup_sql.find("duplicated_songs"));
  EXPECT_NE(std::string::npos, dup_sql.find("INNER JOIN"));
}

TEST(CollectionQuery, BindsStringsAndInlinesIntegers) {
  CollectionQuery query(nullptr, "songs");
  query.SetColumnSpec("ROWID, title");
  query.AddWhere("artist", std::string("Portishead"));
  query.AddWhere("year", 1994);
  query.AddWhere("length", static_cast<int64_t>(180000000000));
  query.AddCompilationRequirement(true);
  query.SetOrderBy("title");
  query.SetLimit(10);
  const std::string sql = query.Sql();
  EXPECT_EQ("SELECT ROWID, title FROM songs WHERE artist = ? AND year = 1994 AND length = 180000000000 AND "
            "+compilation_effective = 1 AND songs.unavailable = 0 ORDER BY title LIMIT 10",
            sql);
  query.AddWhereIn("title", {});
  EXPECT_NE(std::string::npos, query.Sql().find(" AND 0 AND songs.unavailable = 0"));
}

TEST(CollectionFilter, UsesFilterParserOnSongsAndItems) {
  Song match = MakeSong("Roads", "Portishead", "Dummy");
  Song other = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  CollectionFilter filter;
  filter.SetFilterString("artist:Portishead");
  EXPECT_TRUE(filter.Accepts(match));
  EXPECT_FALSE(filter.Accepts(other));
  EXPECT_EQ(1u, filter.FilterSongs({match, other}).size());

  CollectionItem root(CollectionItem::Type::Root);
  CollectionItem *container = root.AddChild(CollectionItem::Type::Container);
  container->display_text = "Portishead";
  CollectionItem *song_item = container->AddChild(CollectionItem::Type::Song);
  song_item->metadata = match;
  CollectionItem other_item(CollectionItem::Type::Song);
  other_item.metadata = other;
  EXPECT_TRUE(filter.AcceptsItem(container));
  EXPECT_FALSE(filter.AcceptsItem(&other_item));
}

TEST(CollectionModel, BuildsGroupedTreeAndPlaylistItem) {
  Song a = MakeSong("Roads", "Portishead", "Dummy");
  Song b = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({a, b}, grouping, false, true, false);
  EXPECT_EQ(2, model.TotalSongs());
  EXPECT_EQ(2, model.TotalArtists());
  EXPECT_EQ(2, model.TotalAlbums());
  EXPECT_EQ(2u, model.Songs().size());
  ASSERT_FALSE(model.root()->children.empty());
  EXPECT_EQ(2u, model.SongsFromItem(model.root()).size());
  const std::string first_label = CollectionItemDelegate::PrimaryText(model.root()->children.front().get());
  EXPECT_TRUE(first_label == "Fleet Foxes" || first_label == "Portishead");

  CollectionPlaylistItem item(a);
  EXPECT_EQ(a.PrettyTitleWithArtist(), item.DisplayText());
  EXPECT_EQ(a.url(), item.url());
}

TEST(CollectionItemDelegate, PrimarySecondaryAndIndent) {
  CollectionItem song(CollectionItem::Type::Song);
  song.metadata = MakeSong("Roads", "Portishead", "Dummy");
  song.container_level = 1;
  EXPECT_EQ("Roads", CollectionItemDelegate::PrimaryText(&song));
  EXPECT_EQ("Portishead · Dummy", CollectionItemDelegate::SecondaryText(&song));
  EXPECT_EQ(2, CollectionItemDelegate::Indent(&song));
  CollectionItem loading(CollectionItem::Type::LoadingIndicator);
  EXPECT_EQ("Loading…", CollectionItemDelegate::PrimaryText(&loading));
}

TEST(CollectionTask, StartsAndFinishesTaskManagerEntry) {
  TaskManager manager;
  {
    CollectionTask task(&manager, "Scan collection");
    EXPECT_EQ(1, task.id());
    EXPECT_EQ(1u, manager.GetTasks().size());
    task.SetProgress(50, 100);
    EXPECT_EQ(50, manager.GetTasks().front().progress);
  }
  EXPECT_TRUE(manager.GetTasks().empty());
}

TEST(CollectionBackend, SongsFromQueryMapsColumnsAndAppliesOptions) {
  const std::string path = "/tmp/strawberry-collection-test-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);

  Song recent = MakeSong("Roads", "Portishead", "Dummy", std::time(nullptr));
  recent.set_directory_id(directory);
  recent.set_track(8);
  const int recent_id = backend.AddOrUpdateSong(recent);
  ASSERT_GT(recent_id, 0);
  backend.SetRating(recent_id, 0.8f);

  Song old = MakeSong("Wandering Star", "Portishead", "Dummy", std::time(nullptr) - 40 * 86400);
  old.set_url("file:///tmp/music/wandering.flac");
  old.set_directory_id(directory);
  backend.AddOrUpdateSong(old);

  Song untagged;
  untagged.set_url("file:///tmp/music/unknown.flac");
  untagged.set_directory_id(directory);
  untagged.set_ctime(std::time(nullptr));
  backend.AddOrUpdateSong(untagged);

  Song duplicate = MakeSong("Roads", "Portishead", "Dummy", std::time(nullptr));
  duplicate.set_url("file:///tmp/music/roads-copy.flac");
  duplicate.set_directory_id(directory);
  backend.AddOrUpdateSong(duplicate);

  CollectionFilterOptions all;
  SongList songs = backend.Songs(all);
  ASSERT_GE(songs.size(), 3u);
  bool found_recent = false;
  for (const Song &song : songs) {
    if (song.title() == "Roads" && song.url() == recent.url()) {
      found_recent = true;
      EXPECT_EQ(180000000000, song.length_nanosec());
      EXPECT_EQ(320, song.bitrate());
      EXPECT_EQ(44100, song.samplerate());
      EXPECT_EQ(16, song.bitdepth());
      EXPECT_NEAR(0.8f, song.rating(), 0.001f);
      EXPECT_GT(song.ctime(), 0);
    }
  }
  EXPECT_TRUE(found_recent);

  CollectionFilterOptions age;
  age.set_max_age(7 * 86400);
  const SongList recent_only = backend.Songs(age);
  for (const Song &song : recent_only) {
    EXPECT_NE("Wandering Star", song.title());
  }

  CollectionFilterOptions untagged_options;
  untagged_options.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  const SongList untagged_songs = backend.Songs(untagged_options);
  ASSERT_EQ(1u, untagged_songs.size());
  EXPECT_TRUE(untagged_songs.front().title().empty());

  CollectionFilterOptions duplicates;
  duplicates.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  const SongList duplicate_songs = backend.Songs(duplicates);
  EXPECT_GE(duplicate_songs.size(), 2u);
  for (const Song &song : duplicate_songs) {
    EXPECT_EQ("Roads", song.title());
  }

  CollectionDirectoryModel directories(&backend);
  ASSERT_EQ(1, directories.Count());
  ASSERT_TRUE(directories.At(0));
  EXPECT_EQ("/tmp/music", directories.At(0)->path);

  unlink(path.c_str());
}
