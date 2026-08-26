#include "collection/collectionautoopen.h"
#include "collection/collectionempty.h"
#include "collection/collectiontreeclick.h"
#include "collection/collectionbackend.h"
#include "collection/collectionbehaviour.h"
#include "collection/collectioncover.h"
#include "collection/collectiondivider.h"
#include "collection/collectioniconcache.h"
#include "collection/collectionstats.h"
#include "collection/collectioncompilation.h"
#include "engine/backendoptions.h"
#include "collection/collectionwatcher.h"
#include "collection/collectiondirectorymodel.h"
#include "collection/collectionfilter.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectionitem.h"
#include "collection/collectionitemdelegate.h"
#include "collection/collectionfiltermenu.h"
#include "collection/collectionmenu.h"
#include "collection/collectionkeyboard.h"
#include "collection/collectionmodel.h"
#include "collection/collectionplaylistitem.h"
#include "collection/collectiontree.h"
#include "collection/collectionquery.h"
#include "collection/collectiontask.h"
#include "core/database.h"
#include "core/taskmanager.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <ctime>
#include <set>
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

TEST(CollectionDivider, KeysAndDisplayMatchQt) {
  EXPECT_TRUE(CollectionDivider::ShouldInsert(true, 0, "P"));
  EXPECT_FALSE(CollectionDivider::ShouldInsert(true, 1, "P"));
  EXPECT_FALSE(CollectionDivider::ShouldInsert(false, 0, "P"));
  EXPECT_FALSE(CollectionDivider::ShouldInsert(true, 0, ""));
  Song song = MakeSong("Roads", "Portishead", "Dummy");
  song.set_year(1994);
  song.set_bitrate(320);
  EXPECT_EQ("P", CollectionDivider::Key(CollectionGrouping::GroupBy::AlbumArtist, song, "Portishead"));
  EXPECT_EQ("0", CollectionDivider::Key(CollectionGrouping::GroupBy::Artist, song, "123 Party"));
  EXPECT_EQ("0-9", CollectionDivider::DisplayText(CollectionGrouping::GroupBy::Artist, "0"));
  EXPECT_EQ("P", CollectionDivider::DisplayText(CollectionGrouping::GroupBy::Artist, "P"));
  EXPECT_EQ("1990", CollectionDivider::Key(CollectionGrouping::GroupBy::Year, song, "1994"));
  EXPECT_EQ("1990", CollectionDivider::DisplayText(CollectionGrouping::GroupBy::Year, "1990"));
  EXPECT_EQ("Unknown", CollectionDivider::DisplayText(CollectionGrouping::GroupBy::Year, "0000"));
  EXPECT_EQ("1994", CollectionDivider::Key(CollectionGrouping::GroupBy::YearAlbum, song, "1994 - Dummy"));
  EXPECT_EQ("0320", CollectionDivider::SortTextForNumber(320));
  EXPECT_EQ("320", CollectionDivider::SortTextForBitrate(320));
  CollectionItem divider(CollectionItem::Type::Divider);
  EXPECT_TRUE(CollectionDivider::IsDivider(&divider));
  EXPECT_TRUE(CollectionItemDelegate::IsDivider(&divider));
}

TEST(CollectionModel, InsertsTopLevelDividersWhenEnabled) {
  Song a = MakeSong("Roads", "Portishead", "Dummy");
  Song b = MakeSong("White Winter Hymnal", "Fleet Foxes", "Fleet Foxes");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({a, b}, grouping, false, true, false, true);
  ASSERT_TRUE(model.root());
  int dividers = 0;
  int containers = 0;
  for (const auto &child : model.root()->children) {
    if (child->type == CollectionItem::Type::Divider) {
      ++dividers;
    }
    if (child->type == CollectionItem::Type::Container) {
      ++containers;
    }
  }
  EXPECT_EQ(2, containers);
  EXPECT_EQ(2, dividers);
}

TEST(CollectionCover, PrettyCoversMatchQtAlbumRows) {
  EXPECT_EQ(32, CollectionCover::kArtHeight);
  EXPECT_STREQ("media-optical-symbolic", CollectionCover::kPlaceholderIcon);
  EXPECT_STREQ("pretty_covers", CollectionSettings::kPrettyCovers);
  EXPECT_TRUE(CollectionSettings::kDefaultPrettyCovers);
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::AlbumDisc;
  grouping.third = CollectionGrouping::GroupBy::None;
  EXPECT_TRUE(CollectionCover::ShouldShowThumb(true, CollectionItem::Type::Container, 1, grouping));
  EXPECT_FALSE(CollectionCover::ShouldShowThumb(false, CollectionItem::Type::Container, 1, grouping));
  EXPECT_FALSE(CollectionCover::ShouldShowThumb(true, CollectionItem::Type::Container, 0, grouping));
  EXPECT_FALSE(CollectionCover::ShouldShowThumb(true, CollectionItem::Type::Song, 1, grouping));
  EXPECT_FALSE(CollectionCover::ShouldShowThumb(true, CollectionItem::Type::Divider, 1, grouping));
  CollectionItem album(CollectionItem::Type::Container);
  album.container_level = 1;
  CollectionItem *song = album.AddChild(CollectionItem::Type::Song);
  song->metadata = MakeSong("Roads", "Portishead", "Dummy");
  song->metadata.set_albumartist("Portishead");
  EXPECT_EQ("Roads", CollectionCover::RepresentativeSong(&album).title());
  EXPECT_EQ("Portishead|Dummy", CollectionCover::CacheKey(&album));
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

TEST(CollectionTree, ExpandKeysAndDragPayload) {
  CollectionItem root(CollectionItem::Type::Root);
  CollectionItem *artist = root.AddChild(CollectionItem::Type::Container);
  artist->key = "portishead";
  artist->display_text = "Portishead";
  artist->container_level = 0;
  CollectionItem *album = artist->AddChild(CollectionItem::Type::Container);
  album->key = "dummy";
  album->display_text = "Dummy";
  album->container_level = 1;
  CollectionItem *song = album->AddChild(CollectionItem::Type::Song);
  song->metadata = MakeSong("Roads", "Portishead", "Dummy");
  song->display_text = "Roads";

  EXPECT_TRUE(CollectionTree::IsExpandable(artist));
  EXPECT_TRUE(CollectionTree::IsExpandable(album));
  EXPECT_FALSE(CollectionTree::IsExpandable(song));
  EXPECT_EQ("portishead:0", CollectionTree::Key(artist));
  EXPECT_EQ("dummy:1", CollectionTree::Key(album));

  std::set<std::string> expanded;
  EXPECT_FALSE(CollectionTree::ShowChildren(artist, false, expanded));
  EXPECT_TRUE(CollectionTree::ShowChildren(artist, true, expanded));
  EXPECT_TRUE(CollectionTree::Toggle(&expanded, artist));
  EXPECT_TRUE(CollectionTree::ShowChildren(artist, false, expanded));
  EXPECT_FALSE(CollectionTree::Toggle(&expanded, artist));
  EXPECT_FALSE(CollectionTree::ShowChildren(artist, false, expanded));

  CollectionTree::CollectExpandableKeys(&root, &expanded);
  EXPECT_EQ(2u, expanded.size());
  EXPECT_NE(expanded.end(), expanded.find(CollectionTree::Key(artist)));
  EXPECT_NE(expanded.end(), expanded.find(CollectionTree::Key(album)));

  SongList songs = {song->metadata};
  EXPECT_EQ(song->metadata.url(), CollectionTree::DragPayload(songs));
  EXPECT_TRUE(CollectionTree::DragPayload({}).empty());
  EXPECT_EQ(1u, CollectionTree::SongsFromItem(&root).size());
  EXPECT_EQ(0, CollectionTree::VisibleSongCount(&root, false, {}));
  EXPECT_EQ(1, CollectionTree::VisibleSongCount(&root, true, {}));
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

TEST(CollectionWatcher, NeedsRescanUsesMtimeAndUnavailable) {
  Song existing;
  existing.set_valid(true);
  existing.set_mtime(100);
  existing.set_filesize(50);
  EXPECT_FALSE(CollectionWatcher::NeedsRescan(existing, 100, 50));
  EXPECT_TRUE(CollectionWatcher::NeedsRescan(existing, 101, 50));
  EXPECT_TRUE(CollectionWatcher::NeedsRescan(existing, 100, 51));
  existing.set_unavailable(true);
  EXPECT_TRUE(CollectionWatcher::NeedsRescan(existing, 100, 50));
  Song missing;
  EXPECT_TRUE(CollectionWatcher::NeedsRescan(missing, 100, 50));
}

TEST(CollectionBackend, MarkMissingUnavailableLeavesSeenSongs) {
  const std::string path = "/tmp/strawberry-collection-unavailable.db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song keep = MakeSong("Keep", "A", "Album");
  keep.set_directory_id(directory);
  keep.set_url("file:///tmp/music/keep.flac");
  const int keep_id = backend.AddOrUpdateSong(keep);
  Song gone = MakeSong("Gone", "A", "Album");
  gone.set_directory_id(directory);
  gone.set_url("file:///tmp/music/gone.flac");
  const int gone_id = backend.AddOrUpdateSong(gone);
  EXPECT_EQ(1, backend.MarkMissingUnavailable(directory, {"file:///tmp/music/keep.flac"}));
  EXPECT_FALSE(backend.SongById(keep_id).unavailable());
  EXPECT_TRUE(backend.SongById(gone_id).unavailable());
  unlink(path.c_str());
}

TEST(CollectionBackend, PersistsRatingPlaycountAndEmbeddedArt) {
  const std::string path = "/tmp/strawberry-collection-rating-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);

  Song song = MakeSong("Rated", "Artist", "Album", std::time(nullptr));
  song.set_directory_id(directory);
  song.set_rating(0.6f);
  song.set_playcount(7);
  song.set_art_embedded(true);
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);

  const Song loaded = backend.SongByUrl(song.url());
  EXPECT_NEAR(0.6f, loaded.rating(), 0.001f);
  EXPECT_EQ(7u, loaded.playcount());
  EXPECT_TRUE(loaded.art_embedded());

  Song update = loaded;
  update.set_title("Rated Again");
  update.set_rating(-1.0f);
  update.set_playcount(0);
  backend.AddOrUpdateSong(update);
  const Song kept = backend.SongByUrl(song.url());
  EXPECT_EQ("Rated Again", kept.title());
  EXPECT_NEAR(0.6f, kept.rating(), 0.001f);
  EXPECT_EQ(7u, kept.playcount());
  EXPECT_TRUE(kept.art_embedded());

  backend.IncrementSkipCount(id);
  backend.IncrementPlayCount(id);
  const Song counted = backend.SongById(id);
  EXPECT_EQ(8u, counted.playcount());
  EXPECT_EQ(1u, counted.skipcount());
  EXPECT_GT(counted.lastplayed(), 0);
  backend.ResetPlayStatistics(id);
  const Song reset = backend.SongById(id);
  EXPECT_EQ(0u, reset.playcount());
  EXPECT_EQ(0u, reset.skipcount());
  EXPECT_LE(reset.lastplayed(), 0);
  unlink(path.c_str());
}

TEST(CollectionBehaviour, DoubleClickAndMenuPlans) {
  using AB = BehaviourSettings::AddBehaviour;
  using PB = BehaviourSettings::PlayBehaviour;
  EXPECT_FALSE(CollectionBehaviour::ShouldPlay(PB::Never, true));
  EXPECT_TRUE(CollectionBehaviour::ShouldPlay(PB::IfStopped, true));
  EXPECT_FALSE(CollectionBehaviour::ShouldPlay(PB::IfStopped, false));
  EXPECT_TRUE(CollectionBehaviour::ShouldPlay(PB::Always, false));

  const auto append = CollectionBehaviour::FromDoubleClick(AB::Append, PB::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::Current, append.destination);
  EXPECT_FALSE(append.clear_current);
  EXPECT_EQ(CollectionBehaviour::QueueMode::None, append.queue);
  EXPECT_FALSE(append.should_play);

  const auto load = CollectionBehaviour::FromDoubleClick(AB::Load, PB::Always, false);
  EXPECT_TRUE(load.clear_current);
  EXPECT_TRUE(load.should_play);

  const auto enqueue = CollectionBehaviour::FromDoubleClick(AB::Enqueue, PB::Never, true);
  EXPECT_EQ(CollectionBehaviour::QueueMode::Append, enqueue.queue);

  const auto created = CollectionBehaviour::FromDoubleClick(AB::OpenInNew, PB::IfStopped, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, created.destination);
  EXPECT_TRUE(created.should_play);

  EXPECT_EQ(CollectionBehaviour::QueueMode::Next, CollectionBehaviour::EnqueueNext().queue);

  const auto replace = CollectionBehaviour::Replace(PB::Always, false);
  EXPECT_TRUE(replace.clear_current);
  EXPECT_EQ(CollectionBehaviour::Destination::Current, replace.destination);
  EXPECT_TRUE(replace.should_play);
}

TEST(CollectionBehaviour, SearchQueryUniqueAndPlaylistName) {
  CollectionItem artist(CollectionItem::Type::Container);
  artist.container_level = 0;
  artist.display_text = "Portishead";
  CollectionItem *album = artist.AddChild(CollectionItem::Type::Container);
  album->container_level = 1;
  album->display_text = "Dummy";
  CollectionItem *track = album->AddChild(CollectionItem::Type::Song);
  track->metadata = MakeSong("Roads", "Portishead", "Dummy");
  track->metadata.set_albumartist("Portishead");

  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  EXPECT_EQ("albumartist:\"Portishead\"", CollectionBehaviour::SearchQuery(&artist, grouping));
  EXPECT_EQ("album:\"Dummy\"", CollectionBehaviour::SearchQuery(album, grouping));
  EXPECT_EQ("title:\"Roads\"", CollectionBehaviour::SearchQuery(track, grouping));

  Song other = MakeSong("Glory Box", "Portishead", "Dummy");
  other.set_url("file:///tmp/music/Roads.flac");
  const SongList unique = CollectionBehaviour::UniqueByUrl({track->metadata, other, track->metadata});
  ASSERT_EQ(1u, unique.size());
  EXPECT_EQ("Dummy", CollectionBehaviour::NewPlaylistName(unique));
}

TEST(CollectionCompilation, EffectiveAndAlbumKeys) {
  EXPECT_EQ(1, CollectionCompilation::Effective(true, false, false, false));
  EXPECT_EQ(1, CollectionCompilation::Effective(false, true, false, false));
  EXPECT_EQ(1, CollectionCompilation::Effective(false, false, true, false));
  EXPECT_EQ(0, CollectionCompilation::Effective(true, true, true, true));
  EXPECT_EQ(0, CollectionCompilation::Effective(false, false, false, false));
  Song a = MakeSong("One", "A", "Album");
  Song b = MakeSong("Two", "A", "Album");
  b.set_url("file:///tmp/music/two.flac");
  Song c = MakeSong("Three", "B", "Album");
  c.set_url("file:///tmp/music/three.flac");
  const auto keys = CollectionCompilation::AlbumArtistKeys({a, b, c});
  ASSERT_EQ(2u, keys.size());
  EXPECT_EQ("Album", keys.front().first);
}

TEST(CollectionBackend, ForceCompilationSetsEffectiveFlag) {
  const std::string path = "/tmp/strawberry-collection-compilation-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song song = MakeSong("Roads", "Portishead", "Dummy");
  song.set_directory_id(directory);
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);
  EXPECT_FALSE(backend.SongById(id).compilation());
  EXPECT_EQ(1, backend.ForceCompilation({song}, true));
  EXPECT_TRUE(backend.SongById(id).compilation());
  EXPECT_EQ(1, backend.ForceCompilation({song}, false));
  EXPECT_FALSE(backend.SongById(id).compilation());
  unlink(path.c_str());
}

TEST(BackendOptions, PlaybinCrossfadeAndPauseFade) {
  EXPECT_STREQ("playbin3", BackendOptions::PlaybinFactory(true));
  EXPECT_STREQ("playbin", BackendOptions::PlaybinFactory(false));
  EXPECT_TRUE(BackendOptions::SameAlbum("Dummy", "Dummy"));
  EXPECT_FALSE(BackendOptions::SameAlbum("Dummy", "Third"));
  EXPECT_FALSE(BackendOptions::SameAlbum("", ""));
  EXPECT_TRUE(BackendOptions::AllowAutoCrossfade(true, true, "A", "B"));
  EXPECT_FALSE(BackendOptions::AllowAutoCrossfade(true, true, "A", "A"));
  EXPECT_TRUE(BackendOptions::AllowAutoCrossfade(true, false, "A", "A"));
  EXPECT_FALSE(BackendOptions::AllowAutoCrossfade(false, false, "A", "B"));
  EXPECT_EQ(0, BackendOptions::FadeDurationMs(false, 250, 250));
  EXPECT_EQ(250, BackendOptions::FadeDurationMs(true, 250, 2000));
  EXPECT_EQ(2000, BackendOptions::FadeDurationMs(true, 0, 2000));
  EXPECT_EQ(4000 * BackendOptions::kNsecPerMsec, BackendOptions::BufferDurationNanosec(4000));
  EXPECT_EQ(0, BackendOptions::BufferDurationNanosec(-10));
  EXPECT_DOUBLE_EQ(0.0, BackendOptions::ClampWatermark(-0.2));
  EXPECT_DOUBLE_EQ(1.0, BackendOptions::ClampWatermark(1.5));
  EXPECT_DOUBLE_EQ(0.33, BackendOptions::ClampWatermark(0.33));
  EXPECT_DOUBLE_EQ(0.5, BackendOptions::VolumeFraction(50, false));
  EXPECT_DOUBLE_EQ(0.0, BackendOptions::VolumeFraction(0, true));
  EXPECT_DOUBLE_EQ(1.0, BackendOptions::VolumeFraction(100, true));
  EXPECT_NEAR(0.31622776601, BackendOptions::VolumeFraction(80, true), 0.0001);
  EXPECT_EQ(2, BackendOptions::EffectiveChannels(true, 2));
  EXPECT_EQ(0, BackendOptions::EffectiveChannels(false, 2));
  EXPECT_EQ(0, BackendOptions::EffectiveChannels(true, 0));
  EXPECT_STREQ("", BackendOptions::SoupForceHttp1(true));
  EXPECT_STREQ("1", BackendOptions::SoupForceHttp1(false));
  EXPECT_EQ(500, BackendOptions::WarmupMs(true, 500));
  EXPECT_EQ(0, BackendOptions::WarmupMs(false, 500));
}

TEST(CollectionFilterMenu, DelayAndBuiltinPresets) {
  EXPECT_FALSE(CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::AlwaysInstant, 1, 200000));
  EXPECT_TRUE(CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::AlwaysDelayed, 1, 10));
  EXPECT_TRUE(CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::DelayedOnLargeLibraries, 2, 100000));
  EXPECT_FALSE(CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::DelayedOnLargeLibraries, 3, 100000));
  EXPECT_FALSE(CollectionFilterMenu::ShouldDelay(CollectionFilterMenu::DelayBehaviour::DelayedOnLargeLibraries, 1, 10));
  EXPECT_EQ(500, CollectionFilterMenu::kFilterDelayMs);
  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  ASSERT_EQ(15u, presets.size());
  EXPECT_TRUE(CollectionFilterMenu::IsAdvancedAction(presets.back()));
  EXPECT_EQ("Group by Album artist/Album", std::string(presets.front().label));
  CollectionGrouping::Grouping album_disc;
  album_disc.first = CollectionGrouping::GroupBy::AlbumArtist;
  album_disc.second = CollectionGrouping::GroupBy::AlbumDisc;
  album_disc.third = CollectionGrouping::GroupBy::None;
  EXPECT_EQ(1, CollectionFilterMenu::MatchingPresetIndex(album_disc, presets));
  CollectionGrouping::Grouping custom;
  custom.first = CollectionGrouping::GroupBy::Year;
  custom.second = CollectionGrouping::GroupBy::Composer;
  custom.third = CollectionGrouping::GroupBy::None;
  EXPECT_EQ(-1, CollectionFilterMenu::MatchingPresetIndex(custom, presets));
}

TEST(CollectionKeyboard, FromKeyAndMoveAction) {
  EXPECT_EQ(CollectionKeyboard::Action::Activate, CollectionKeyboard::FromKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(CollectionKeyboard::Action::MoveUp, CollectionKeyboard::FromKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(CollectionKeyboard::Action::MoveDown, CollectionKeyboard::FromKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(CollectionKeyboard::Action::Expand, CollectionKeyboard::FromKey(ListBoxKeyboard::kRight));
  EXPECT_EQ(CollectionKeyboard::Action::Collapse, CollectionKeyboard::FromKey(ListBoxKeyboard::kLeft));
  EXPECT_EQ(CollectionKeyboard::Action::Home, CollectionKeyboard::FromKey(ListBoxKeyboard::kHome));
  EXPECT_EQ(CollectionKeyboard::Action::End, CollectionKeyboard::FromKey(ListBoxKeyboard::kEnd));
  EXPECT_EQ(ListBoxKeyboard::Action::MoveDown, CollectionKeyboard::MoveAction(CollectionKeyboard::Action::MoveDown));
  EXPECT_EQ(ListBoxKeyboard::Action::None, CollectionKeyboard::MoveAction(CollectionKeyboard::Action::Expand));
}

TEST(CollectionMenu, CatalogAndEmptySelection) {
  EXPECT_EQ(16, CollectionMenu::ItemCount());
  EXPECT_EQ(CollectionMenu::Action::Organize, CollectionMenu::FromId("organize"));
  EXPECT_EQ(CollectionMenu::Action::EditTracks, CollectionMenu::FromId("edit-tracks"));
  EXPECT_STREQ("win.collection-copy-device", CollectionMenu::WinAction(CollectionMenu::Action::CopyToDevice));
  EXPECT_STREQ("", CollectionMenu::WinAction(CollectionMenu::Action::GroupBy));
  const auto empty = CollectionMenu::VisibleItems(CollectionMenu::Analyze({}));
  EXPECT_EQ(1u, empty.size());
  EXPECT_TRUE(CollectionMenu::Contains(empty, CollectionMenu::Action::GroupBy));
}

TEST(CollectionMenu, EditTrackVsTracksAndOrganize) {
  Song one(Song::Source::Collection);
  one.set_valid(true);
  one.set_url("file:///music/a.flac");
  Song two = one;
  two.set_url("file:///music/b.flac");
  const auto single = CollectionMenu::VisibleItems(CollectionMenu::Analyze({one}, true, true));
  EXPECT_TRUE(CollectionMenu::Contains(single, CollectionMenu::Action::EditTrack));
  EXPECT_FALSE(CollectionMenu::Contains(single, CollectionMenu::Action::EditTracks));
  EXPECT_TRUE(CollectionMenu::Contains(single, CollectionMenu::Action::Organize));
  EXPECT_TRUE(CollectionMenu::Contains(single, CollectionMenu::Action::CopyToDevice));
  EXPECT_TRUE(CollectionMenu::Contains(single, CollectionMenu::Action::Rescan));
  EXPECT_TRUE(CollectionMenu::Contains(single, CollectionMenu::Action::DeleteFiles));
  EXPECT_EQ("Edit track information...", CollectionMenu::LabelFor(CollectionMenu::Action::EditTrack));

  const auto multi = CollectionMenu::VisibleItems(CollectionMenu::Analyze({one, two}, true, true));
  EXPECT_FALSE(CollectionMenu::Contains(multi, CollectionMenu::Action::EditTrack));
  EXPECT_TRUE(CollectionMenu::Contains(multi, CollectionMenu::Action::EditTracks));
  EXPECT_EQ("Edit tracks information...", CollectionMenu::LabelFor(CollectionMenu::Action::EditTracks));
}

TEST(CollectionMenu, MixedEditableHidesOrganize) {
  Song editable(Song::Source::Collection);
  editable.set_valid(true);
  editable.set_url("file:///music/a.flac");
  Song stream(Song::Source::Tidal);
  stream.set_valid(true);
  stream.set_url("tidal://track/1");
  const auto mixed = CollectionMenu::VisibleItems(CollectionMenu::Analyze({editable, stream}, true, false));
  EXPECT_FALSE(CollectionMenu::Contains(mixed, CollectionMenu::Action::Organize));
  EXPECT_FALSE(CollectionMenu::Contains(mixed, CollectionMenu::Action::CopyToDevice));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::Rescan));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::EditTrack));
  EXPECT_FALSE(CollectionMenu::CopyToDeviceEnabled(CollectionMenu::Analyze({editable}, true, false)));
}

TEST(CollectionIconCache, SizeLabelsSafeKeysAndTempStore) {
  EXPECT_STREQ("pixmapcache", CollectionIconCache::kPixmapDiskCacheDir);
  EXPECT_EQ(160 * 1024, CollectionIconCache::MaximumCacheSize(160, 0));
  EXPECT_EQ(360LL * 1024 * 1024, CollectionIconCache::MaximumCacheSize(360, 1));
  EXPECT_EQ("empty", CollectionIconCache::InUseLabel(0));
  EXPECT_NE(std::string("empty"), CollectionIconCache::InUseLabel(2048));
  EXPECT_EQ("cover", CollectionIconCache::SafeKey(""));
  EXPECT_NE(std::string::npos, CollectionIconCache::SafeKey("a|b").find("%7C"));
  const std::string dir = FileUtils::Join("/tmp", "strawberry-icon-cache-" + std::to_string(getpid()));
  CollectionIconCache::Clear(dir);
  EXPECT_EQ(0, CollectionIconCache::DiskCacheBytes(dir));
  EXPECT_TRUE(CollectionIconCache::Write("album", "imagedata", dir, 1024));
  EXPECT_EQ("imagedata", CollectionIconCache::Read("album", dir));
  EXPECT_GT(CollectionIconCache::DiskCacheBytes(dir), 0);
  EXPECT_FALSE(CollectionIconCache::Write("other", "xxxxxxxxxxxxxxxx", dir, 8));
  CollectionIconCache::Clear(dir);
  EXPECT_EQ(0, CollectionIconCache::DiskCacheBytes(dir));
}

TEST(CollectionAutoOpen, DrillsSingleChildAndMatchesQtBudget) {
  EXPECT_EQ(50, CollectionAutoOpen::kRowsToShow);
  EXPECT_FALSE(CollectionSettings::kDefaultAutoOpen);

  CollectionItem root(CollectionItem::Type::Root);
  CollectionItem *artist = root.AddChild(CollectionItem::Type::Container);
  artist->key = "Portishead";
  artist->container_level = 0;
  CollectionItem *album = artist->AddChild(CollectionItem::Type::Container);
  album->key = "Dummy";
  album->container_level = 1;
  CollectionItem *song = album->AddChild(CollectionItem::Type::Song);
  song->metadata = MakeSong("Roads", "Portishead", "Dummy");

  EXPECT_EQ(1, CollectionAutoOpen::ChildCount(artist));
  EXPECT_FALSE(CollectionAutoOpen::ShouldDrillInto(false, artist));
  EXPECT_TRUE(CollectionAutoOpen::ShouldDrillInto(true, artist));
  EXPECT_TRUE(CollectionAutoOpen::ShouldDrillInto(true, album));
  EXPECT_EQ(album, CollectionAutoOpen::SoleChild(artist));
  EXPECT_EQ(song, CollectionAutoOpen::SoleChild(album));

  const std::vector<std::string> drilled = CollectionAutoOpen::DrillKeys(true, artist);
  ASSERT_EQ(1U, drilled.size());
  EXPECT_EQ(CollectionTree::Key(album), drilled.front());
  EXPECT_TRUE(CollectionAutoOpen::DrillKeys(false, artist).empty());

  std::set<std::string> expanded;
  CollectionTree::Toggle(&expanded, artist);
  CollectionAutoOpen::ApplyDrill(&expanded, true, artist);
  EXPECT_TRUE(expanded.count(CollectionTree::Key(artist)));
  EXPECT_TRUE(expanded.count(CollectionTree::Key(album)));
  EXPECT_FALSE(expanded.count(CollectionTree::Key(song)));

  CollectionItem wide(CollectionItem::Type::Root);
  for (int i = 0; i < 3; ++i) {
    CollectionItem *node = wide.AddChild(CollectionItem::Type::Container);
    node->key = "Artist" + std::to_string(i);
    node->container_level = 0;
    CollectionItem *child = node->AddChild(CollectionItem::Type::Container);
    child->key = "Album" + std::to_string(i);
    child->container_level = 1;
    child->AddChild(CollectionItem::Type::Song)->metadata = MakeSong("Track", node->key, child->key);
  }
  EXPECT_TRUE(CollectionAutoOpen::RecursivelyExpandKeys(&wide, false).empty());
  const std::vector<std::string> opened = CollectionAutoOpen::RecursivelyExpandKeys(&wide, true);
  ASSERT_EQ(6U, opened.size());
  EXPECT_EQ(CollectionTree::Key(wide.children.front().get()), opened.front());

  CollectionItem crowded(CollectionItem::Type::Root);
  for (int i = 0; i < 30; ++i) {
    CollectionItem *node = crowded.AddChild(CollectionItem::Type::Container);
    node->key = "N" + std::to_string(i);
    node->container_level = 0;
    node->AddChild(CollectionItem::Type::Song)->metadata = MakeSong("T", node->key, "A");
  }
  EXPECT_TRUE(CollectionAutoOpen::RecursivelyExpandKeys(&crowded, true).empty());
}

TEST(CollectionEmpty, UsesQtCopyAndOpensOnPrimaryClick) {
  EXPECT_STREQ("Your collection is empty!", CollectionEmpty::Title());
  EXPECT_STREQ("Click here to add some music", CollectionEmpty::Hint());
  EXPECT_STREQ("The streaming collection is empty!", CollectionEmpty::StreamingTitle());
  EXPECT_STREQ("Click here to retrieve music", CollectionEmpty::StreamingHint());
  EXPECT_STREQ("No matches", CollectionEmpty::NoMatches());
  EXPECT_STREQ("/org/strawberrymusicplayer/Strawberry/pictures/nomusic.png", CollectionEmpty::kResourcePath);
  EXPECT_TRUE(CollectionEmpty::IsEmptyCollection(0));
  EXPECT_FALSE(CollectionEmpty::IsEmptyCollection(1));
  EXPECT_TRUE(CollectionEmpty::OpensOnPrimaryClick(true, CollectionTreeClick::kPrimaryButton));
  EXPECT_FALSE(CollectionEmpty::OpensOnPrimaryClick(true, CollectionTreeClick::kMiddleButton));
  EXPECT_FALSE(CollectionEmpty::OpensOnPrimaryClick(false, CollectionTreeClick::kPrimaryButton));
}

TEST(CollectionTreeClick, MatchesQtAutoExpandingTreeView) {
  EXPECT_EQ(CollectionTreeClick::Action::ToggleExpand, CollectionTreeClick::FromPress(CollectionTreeClick::kPrimaryButton, 1, static_cast<GdkModifierType>(0)));
  EXPECT_EQ(CollectionTreeClick::Action::None, CollectionTreeClick::FromPress(CollectionTreeClick::kPrimaryButton, 2, static_cast<GdkModifierType>(0)));
  EXPECT_EQ(CollectionTreeClick::Action::None, CollectionTreeClick::FromPress(CollectionTreeClick::kPrimaryButton, 1, GDK_CONTROL_MASK));
  EXPECT_EQ(CollectionTreeClick::Action::None, CollectionTreeClick::FromPress(CollectionTreeClick::kPrimaryButton, 1, GDK_SHIFT_MASK));
  EXPECT_EQ(CollectionTreeClick::Action::Enqueue, CollectionTreeClick::FromPress(CollectionTreeClick::kMiddleButton, 1, static_cast<GdkModifierType>(0)));
  EXPECT_EQ(CollectionTreeClick::Action::Enqueue, CollectionTreeClick::FromPress(CollectionTreeClick::kMiddleButton, 1, GDK_CONTROL_MASK));
  EXPECT_EQ(CollectionTreeClick::Action::None, CollectionTreeClick::FromPress(CollectionTreeClick::kSecondaryButton, 1, static_cast<GdkModifierType>(0)));
  EXPECT_TRUE(CollectionTreeClick::ShouldToggleFromRowClick(false, true));
  EXPECT_FALSE(CollectionTreeClick::ShouldToggleFromRowClick(true, true));
  EXPECT_FALSE(CollectionTreeClick::ShouldToggleFromRowClick(false, false));
  EXPECT_TRUE(CollectionTreeClick::SelectRowBeforeEnqueue(false));
  EXPECT_FALSE(CollectionTreeClick::SelectRowBeforeEnqueue(true));
}

TEST(CollectionStats, WritesLocalSongsAndKeepsQtCopy) {
  EXPECT_STREQ("Write all playcounts and ratings to files", CollectionStats::ConfirmTitle());
  EXPECT_STREQ("Are you sure you want to write song playcounts and ratings to file for all songs in your collection?",
               CollectionStats::ConfirmText());
  EXPECT_STREQ("Save playcounts and ratings to files now", CollectionStats::SaveNowLabel());
  EXPECT_STREQ("Saving playcounts and ratings", CollectionStats::TaskName());
  EXPECT_STREQ("Current disk cache in use:", CollectionStats::CacheInUseTitle());
  EXPECT_STREQ("Clear Disk Cache", CollectionStats::ClearCacheLabel());
  const Song local = MakeSong("Roads", "Portishead", "Dummy");
  EXPECT_TRUE(CollectionStats::ShouldWriteStatistics(local));
  Song stream(Song::Source::Stream);
  stream.set_url("https://example/a.mp3");
  EXPECT_FALSE(CollectionStats::ShouldWriteStatistics(stream));
  EXPECT_EQ(1, CollectionStats::SongsToWrite({local, stream}));
}
