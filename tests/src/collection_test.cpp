#include "collection/collectionfullrescan.h"
#include "collection/collectioncompilationdetect.h"
#include "collection/collectionsongpatch.h"
#include "organize/organizepathnotify.h"
#include "collection/skipcounteligibility.h"
#include "collection/collectionautoopen.h"
#include "collection/collectionempty.h"
#include "collection/collectionfilterkeyboard.h"
#include "collection/collectiontreeclick.h"
#include "collection/collectionbackend.h"
#include "collection/collectionunavailable.h"
#include "playlist/playlist.h"
#include "playlist/playlistcollectionsync.h"
#include "collection/collectionsubdirectory.h"
#include "collection/collectionbehaviour.h"
#include "collection/collectionsearchsync.h"
#include "collection/collectioncover.h"
#include "collection/collectiondivider.h"
#include "collection/collectioniconcache.h"
#include "collection/collectionsearchlabels.h"
#include "collection/collectionstats.h"
#include "subsonic/subsonicsettingsactions.h"
#include "collection/collectioncompilation.h"
#include "engine/backendoptions.h"
#include "engine/engineexclusive.h"
#include "core/exitfade.h"
#include "engine/enginefade.h"
#include "engine/engineplay.h"
#include "engine/engineseek.h"
#include "collection/collectionalbumart.h"
#include "collection/collectionartpersist.h"
#include "collection/collectioncuescan.h"
#include "collection/collectiondirectoryart.h"
#include "collection/collectionexpire.h"
#include "collection/collectionfingerprintmatch.h"
#include "collection/collectionrescanreason.h"
#include "collection/collectionunavailablerestore.h"
#include "collection/collectionscanprogress.h"
#include "collection/collectiontagsave.h"
#include "collection/collectionscandelay.h"
#include "collection/collectionscangates.h"
#include "collection/collectionwatcher.h"
#include "core/songuserdatamerge.h"
#include "collection/collectiondirectorymodel.h"
#include "collection/collectionfocus.h"
#include "collection/collectionfilter.h"
#include "collection/collectionfilteroptions.h"
#include "collection/collectionitem.h"
#include "collection/collectionitemdelegate.h"
#include "collection/collectionfilterchoices.h"
#include "collection/collectionfiltermenu.h"
#include "collection/collectiongroupingsave.h"
#include "collection/collectionmenu.h"
#include "collection/collectionkeyboard.h"
#include "collection/collectionchangenotify.h"
#include "collection/collectionincremental.h"
#include "collection/playcountincrement.h"
#include "collection/collectionmodel.h"
#include "collection/collectionmodelmerge.h"
#include "collection/collectionplaylistitem.h"
#include "collection/collectiontree.h"
#include "collection/collectionquery.h"
#include "collection/collectiontask.h"
#include "core/database.h"
#include "core/taskmanager.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include <ctime>
#include <map>
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

TEST(CollectionFilterOptions, TextSearchEnabledOnlyForAll) {
  CollectionFilterOptions options;
  EXPECT_TRUE(options.TextSearchEnabled());
  EXPECT_TRUE(CollectionFilterOptions::TextSearchEnabled(CollectionFilterOptions::FilterMode::All));
  options.set_filter_mode(CollectionFilterOptions::FilterMode::Duplicates);
  EXPECT_FALSE(options.TextSearchEnabled());
  options.set_filter_mode(CollectionFilterOptions::FilterMode::Untagged);
  EXPECT_FALSE(options.TextSearchEnabled());
  EXPECT_FALSE(CollectionFilterOptions::TextSearchEnabled(CollectionFilterOptions::FilterMode::Duplicates));
  EXPECT_FALSE(CollectionFilterOptions::TextSearchEnabled(CollectionFilterOptions::FilterMode::Untagged));
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

TEST(CollectionModelMerge, AddsRemovesAndUpdatesById) {
  Song first = MakeSong("Roads", "Portishead", "Dummy");
  first.set_id(1);
  Song second = MakeSong("Glory Box", "Portishead", "Dummy");
  second.set_id(2);
  SongList songs = CollectionModelMerge::Add({}, {first, second});
  ASSERT_EQ(2u, songs.size());
  Song replacement = first;
  replacement.set_title("Roads (live)");
  songs = CollectionModelMerge::Add(songs, {replacement});
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ("Roads (live)", songs.front().title());
  Song extra = MakeSong("Wandering Star", "Portishead", "Dummy");
  extra.set_id(3);
  CollectionModelUpdate add;
  add.type = CollectionModelUpdateType::AddSongs;
  add.songs = {extra};
  songs = CollectionModelMerge::Apply(songs, add);
  EXPECT_EQ(3u, songs.size());
  Song updated = second;
  updated.set_playcount(4);
  CollectionModelUpdate patch;
  patch.type = CollectionModelUpdateType::UpdateSongs;
  patch.songs = {updated};
  songs = CollectionModelMerge::Apply(songs, patch);
  EXPECT_EQ(4, songs[1].playcount());
  CollectionModelUpdate remove;
  remove.type = CollectionModelUpdateType::RemoveSongs;
  remove.songs = {first};
  songs = CollectionModelMerge::Apply(songs, remove);
  ASSERT_EQ(2u, songs.size());
  EXPECT_EQ(2, songs.front().id());
  EXPECT_FALSE(CollectionIncremental::ShouldApply(true));
  EXPECT_TRUE(CollectionIncremental::ShouldApply(false));
  CollectionModelUpdate reset = CollectionIncremental::Make(CollectionModelUpdateType::Reset, {first});
  EXPECT_EQ(CollectionModelUpdateType::Reset, reset.type);
  EXPECT_EQ(1u, reset.songs.size());
}

TEST(CollectionModel, ApplyUpdateMergesWithoutResetList) {
  Song first = MakeSong("Roads", "Portishead", "Dummy");
  first.set_id(11);
  Song second = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  second.set_id(12);
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({first}, grouping, false, true, false);
  EXPECT_EQ(1, model.TotalSongs());
  CollectionModelUpdate add;
  add.type = CollectionModelUpdateType::AddSongs;
  add.songs = {second};
  model.ApplyUpdate(add);
  EXPECT_EQ(2, model.TotalSongs());
  EXPECT_EQ(2u, model.model_songs().size());
  Song patched = first;
  patched.set_playcount(7);
  CollectionModelUpdate update;
  update.type = CollectionModelUpdateType::UpdateSongs;
  update.songs = {patched};
  model.ApplyUpdate(update);
  ASSERT_EQ(2u, model.model_songs().size());
  EXPECT_EQ(7, model.model_songs().front().playcount());
  CollectionModelUpdate remove;
  remove.type = CollectionModelUpdateType::RemoveSongs;
  remove.songs = {second};
  model.ApplyUpdate(remove);
  EXPECT_EQ(1, model.TotalSongs());
  EXPECT_EQ(11, model.model_songs().front().id());
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
  EXPECT_TRUE(CollectionItemDelegate::ShouldShowTooltip("Roads"));
  EXPECT_FALSE(CollectionItemDelegate::ShouldShowTooltip({}));
  EXPECT_EQ("Roads", CollectionItemDelegate::TooltipText("Roads"));
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

const CollectionItem *FindSongItem(const CollectionItem *item, const Song &song) {
  if (!item) {
    return nullptr;
  }
  if (item->type == CollectionItem::Type::Song && item->metadata == song) {
    return item;
  }
  for (const auto &child : item->children) {
    if (const CollectionItem *found = FindSongItem(child.get(), song)) {
      return found;
    }
  }
  return nullptr;
}

TEST(CollectionFocus, CaptureAndRestoreSongAcrossRebuild) {
  Song roads = MakeSong("Roads", "Portishead", "Dummy");
  Song helpless = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({roads, helpless}, grouping, false, true, false);
  const CollectionItem *item = FindSongItem(model.root(), roads);
  ASSERT_TRUE(item);
  EXPECT_TRUE(CollectionFocus::ShouldSave(item->type));

  CollectionFocus::State state;
  CollectionFocus::Capture(item, &state);
  EXPECT_TRUE(CollectionFocus::ShouldRestore(state));
  EXPECT_EQ(roads.url(), state.last_selected_song.url());
  EXPECT_TRUE(state.last_selected_container.empty());
  EXPECT_FALSE(state.last_selected_path.empty());

  model.Reset({roads, helpless}, grouping, false, true, false);
  const CollectionItem *found = CollectionFocus::FindTarget(model.root(), state);
  ASSERT_TRUE(found);
  EXPECT_EQ(roads.url(), found->metadata.url());
  const std::set<std::string> keys = CollectionFocus::ExpandKeys(model.root(), state);
  EXPECT_FALSE(keys.empty());
  EXPECT_TRUE(CollectionFocus::NeedsExpand({}, keys));
  std::set<std::string> expanded;
  CollectionFocus::MergeExpand(&expanded, keys);
  EXPECT_FALSE(CollectionFocus::NeedsExpand(expanded, keys));
}

TEST(CollectionFocus, RestoreContainerAndMissingSong) {
  Song roads = MakeSong("Roads", "Portishead", "Dummy");
  Song glory = MakeSong("Glory Box", "Portishead", "Dummy");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({roads, glory}, grouping, false, true, false);
  const CollectionItem *song = FindSongItem(model.root(), roads);
  ASSERT_TRUE(song);
  ASSERT_TRUE(song->parent);
  CollectionFocus::State container_state;
  CollectionFocus::Capture(song->parent, &container_state);
  EXPECT_FALSE(container_state.last_selected_container.empty());
  EXPECT_TRUE(container_state.last_selected_song.url().empty());

  model.Reset({roads, glory}, grouping, false, true, false);
  const CollectionItem *container = CollectionFocus::FindTarget(model.root(), container_state);
  ASSERT_TRUE(container);
  EXPECT_EQ(container_state.last_selected_container, container->sort_text);

  CollectionFocus::State missing;
  CollectionFocus::Capture(song, &missing);
  Song other = MakeSong("Wanderlust", "Bjork", "Homogenic");
  model.Reset({other}, grouping, false, true, false);
  EXPECT_EQ(nullptr, CollectionFocus::FindTarget(model.root(), missing));
  EXPECT_TRUE(CollectionFocus::ExpandKeys(model.root(), missing).empty());
  CollectionFocus::State empty;
  EXPECT_FALSE(CollectionFocus::ShouldRestore(empty));
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

TEST(CollectionSubdirectoryScan, SkipsUnchangedMtime) {
  EXPECT_TRUE(CollectionSubdirectoryScan::ShouldSkip(100, 100, false));
  EXPECT_FALSE(CollectionSubdirectoryScan::ShouldSkip(100, 101, false));
  EXPECT_FALSE(CollectionSubdirectoryScan::ShouldSkip(100, 100, true));
  EXPECT_FALSE(CollectionSubdirectoryScan::ShouldSkip(-1, 100, false));
  EXPECT_EQ("/tmp/music/album", CollectionSubdirectoryScan::ImmediateParent("/tmp/music/album/track.flac"));
  CollectionSubdirectory stored;
  stored.path = "/tmp/music/album";
  stored.mtime = 42;
  EXPECT_EQ(42, CollectionSubdirectoryScan::StoredMtime({stored}, "/tmp/music/album"));
  EXPECT_EQ(-1, CollectionSubdirectoryScan::StoredMtime({stored}, "/tmp/other"));
}

TEST(CollectionBackend, SubdirsRoundTrip) {
  const std::string path = "/tmp/strawberry-collection-subdirs.db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  CollectionSubdirectory album;
  album.directory_id = directory;
  album.path = "/tmp/music/album";
  album.mtime = 123;
  backend.AddOrUpdateSubdirs(directory, {album});
  const auto loaded = backend.SubdirsInDirectory(directory);
  ASSERT_EQ(1u, loaded.size());
  EXPECT_EQ("/tmp/music/album", loaded.front().path);
  EXPECT_EQ(123, loaded.front().mtime);
  backend.AddOrUpdateSubdirs(directory, {});
  EXPECT_TRUE(backend.SubdirsInDirectory(directory).empty());
  backend.RemoveDirectory(directory);
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

TEST(CollectionRescanReason, ForcesRescanWhenFingerprintOrLoudnessMissing) {
  Song song;
  song.set_valid(true);
  song.set_mtime(100);
  song.set_filesize(50);
  EXPECT_FALSE(CollectionRescanReason::NeedsAnalysisRescan(song, false, false));
  EXPECT_TRUE(CollectionRescanReason::MissingFingerprint(song, true));
  EXPECT_TRUE(CollectionRescanReason::NeedsAnalysisRescan(song, true, false));
  song.set_fingerprint("abc");
  EXPECT_FALSE(CollectionRescanReason::NeedsAnalysisRescan(song, true, false));
  EXPECT_TRUE(CollectionRescanReason::MissingLoudness(song, true));
  EXPECT_TRUE(CollectionRescanReason::NeedsAnalysisRescan(song, false, true));
  song.set_ebur128_integrated_loudness_lufs(-12.0);
  EXPECT_TRUE(CollectionRescanReason::NeedsAnalysisRescan(song, false, true));
  song.set_ebur128_loudness_range_lu(6.0);
  EXPECT_FALSE(CollectionRescanReason::NeedsAnalysisRescan(song, true, true));
  EXPECT_FALSE(CollectionWatcher::NeedsRescan(song, 100, 50, true, true, CollectionCueScan::Change::None));
  EXPECT_TRUE(CollectionWatcher::NeedsRescan(song, 100, 50, false, false, CollectionCueScan::Change::Added));
}

TEST(CollectionCueScan, DetectsAddChangeDeleteAndEffectiveMtime) {
  EXPECT_EQ(CollectionCueScan::Change::Added, CollectionCueScan::DetectCueChange(false, {}, "/tmp/album.cue", 10));
  EXPECT_EQ(CollectionCueScan::Change::Changed, CollectionCueScan::DetectCueChange(true, "/tmp/old.cue", "/tmp/new.cue", 10));
  EXPECT_EQ(CollectionCueScan::Change::Deleted, CollectionCueScan::DetectCueChange(true, "/tmp/old.cue", {}, 0));
  EXPECT_EQ(CollectionCueScan::Change::None, CollectionCueScan::DetectCueChange(true, "/tmp/album.cue", "/tmp/album.cue", 10));
  EXPECT_TRUE(CollectionCueScan::CueForcesRescan(CollectionCueScan::Change::Added));
  EXPECT_FALSE(CollectionCueScan::CueForcesRescan(CollectionCueScan::Change::None));
  EXPECT_EQ(200, CollectionCueScan::EffectiveMtime(100, 200));
  EXPECT_EQ(150, CollectionCueScan::EffectiveMtime(150, 0));
}

TEST(SongUserDataMerge, PreservesPlaycountRatingSkipAndArtUnlessOverwrite) {
  Song incoming;
  incoming.set_playcount(0);
  incoming.set_rating(-1.0f);
  incoming.set_skipcount(0);
  incoming.set_lastplayed(-1);
  Song existing;
  existing.set_playcount(9);
  existing.set_rating(0.8f);
  existing.set_skipcount(3);
  existing.set_lastplayed(123);
  existing.set_art_manual("file:///cover.jpg");
  existing.set_art_unset(false);
  SongUserDataMerge::Merge(&incoming, existing, true, true);
  EXPECT_EQ(9u, incoming.playcount());
  EXPECT_NEAR(0.8f, incoming.rating(), 0.001f);
  EXPECT_EQ(3u, incoming.skipcount());
  EXPECT_EQ(123, incoming.lastplayed());
  EXPECT_EQ("file:///cover.jpg", incoming.art_manual());

  Song overwrite;
  overwrite.set_playcount(1);
  overwrite.set_rating(0.2f);
  SongUserDataMerge::Merge(&overwrite, existing, false, false);
  EXPECT_EQ(1u, overwrite.playcount());
  EXPECT_NEAR(0.2f, overwrite.rating(), 0.001f);
  EXPECT_EQ(3u, overwrite.skipcount());
  existing.set_compilation_on(true);
  existing.set_compilation_off(false);
  SongUserDataMerge::Merge(&incoming, existing, true, true);
  EXPECT_TRUE(incoming.compilation_on());
  EXPECT_FALSE(incoming.compilation_off());
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

TEST(CollectionUnavailable, MissingAndNotifyMatchQt) {
  Song seen;
  seen.set_url("file:///keep.flac");
  Song missing;
  missing.set_url("file:///gone.flac");
  Song already;
  already.set_url("file:///old.flac");
  already.set_unavailable(true);
  EXPECT_FALSE(CollectionUnavailable::IsMissing(seen, {"file:///keep.flac"}));
  EXPECT_TRUE(CollectionUnavailable::IsMissing(missing, {"file:///keep.flac"}));
  EXPECT_FALSE(CollectionUnavailable::IsMissing(already, {}));
  EXPECT_TRUE(CollectionUnavailable::MarkedCopy(missing).unavailable());
  EXPECT_FALSE(CollectionUnavailable::ShouldNotify({}));
  EXPECT_TRUE(CollectionUnavailable::ShouldNotify({missing}));
}

TEST(CollectionBackend, MarkMissingUnavailableNotifiesPlaylists) {
  const std::string path = "/tmp/strawberry-collection-unavailable-notify-" + std::to_string(getpid()) + ".db";
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
  SongList changed;
  SongList deleted;
  backend.SongsChanged.Connect([&changed](const SongList &songs) { changed = songs; });
  backend.SongsDeleted.Connect([&deleted](const SongList &songs) { deleted = songs; });
  EXPECT_EQ(1, backend.MarkMissingUnavailable(directory, {"file:///tmp/music/keep.flac"}));
  ASSERT_EQ(1u, changed.size());
  EXPECT_EQ(gone_id, changed.front().id());
  EXPECT_TRUE(changed.front().unavailable());
  ASSERT_EQ(1u, deleted.size());
  EXPECT_EQ(gone_id, deleted.front().id());
  Playlist playlist;
  Song row = backend.SongById(gone_id);
  row.set_unavailable(false);
  playlist.AppendSongs({row});
  EXPECT_EQ(1, PlaylistCollectionSync::PatchAll({&playlist}, changed));
  EXPECT_TRUE(playlist.song(0).unavailable());
  EXPECT_FALSE(backend.SongById(keep_id).unavailable());
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

  Song collection = MakeSong("Roads", "Portishead", "Dummy");
  collection.set_source(Song::Source::Collection);
  Song tidal(Song::Source::Tidal);
  tidal.set_title("Roads");
  tidal.set_artist("Portishead");
  tidal.set_album("Dummy");
  EXPECT_TRUE(CollectionBehaviour::ShowInCollectionQuery(tidal).empty());
  EXPECT_EQ("artist:Portishead album:Dummy", CollectionBehaviour::ShowInCollectionQuery(collection));
  EXPECT_EQ("artist:Portishead album:Dummy", CollectionBehaviour::ShowInCollectionQuery({tidal, collection}));
  EXPECT_TRUE(CollectionBehaviour::FirstCollectionSong({tidal}).url().empty());
  EXPECT_TRUE(CollectionSearchSync::ShouldUpdateEntry(true));
  EXPECT_FALSE(CollectionSearchSync::ShouldUpdateEntry(false));
  EXPECT_TRUE(CollectionSearchSync::TextDiffers("", "artist:Portishead album:Dummy"));
  EXPECT_FALSE(CollectionSearchSync::TextDiffers("artist:Portishead album:Dummy", "artist:Portishead album:Dummy"));

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

TEST(CollectionCompilationDetect, MarksMultiArtistAlbums) {
  EXPECT_FALSE(CollectionCompilationDetect::IsCompilationAlbum(1));
  EXPECT_TRUE(CollectionCompilationDetect::IsCompilationAlbum(2));
}

TEST(CollectionBackend, UpdateCompilationsDetectsMixedArtists) {
  const std::string path = "/tmp/strawberry-collection-detect-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song a = MakeSong("One", "Artist A", "Shared");
  a.set_directory_id(directory);
  a.set_url("file:///tmp/music/one.flac");
  Song b = MakeSong("Two", "Artist B", "Shared");
  b.set_directory_id(directory);
  b.set_url("file:///tmp/music/two.flac");
  ASSERT_GT(backend.AddOrUpdateSong(a), 0);
  ASSERT_GT(backend.AddOrUpdateSong(b), 0);
  backend.UpdateCompilations();
  EXPECT_TRUE(backend.SongByUrl("file:///tmp/music/one.flac").compilation());
  EXPECT_TRUE(backend.SongByUrl("file:///tmp/music/two.flac").compilation());
  unlink(path.c_str());
}

TEST(OrganizePathNotify, GatesCollectionMoves) {
  Song collection;
  collection.set_id(4);
  collection.set_source(Song::Source::Collection);
  EXPECT_TRUE(OrganizePathNotify::ShouldNotify(true, collection, true));
  EXPECT_FALSE(OrganizePathNotify::ShouldNotify(false, collection, true));
  Song local;
  local.set_id(4);
  local.set_source(Song::Source::LocalFile);
  EXPECT_FALSE(OrganizePathNotify::ShouldNotify(true, local, true));
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
  EXPECT_FALSE(BackendOptions::AllowAutoCrossfade(true, true, "A", "B", true));
  EXPECT_TRUE(BackendOptions::SuppressSameAlbumCrossfade(true, true, true));
  EXPECT_FALSE(BackendOptions::SuppressSameAlbumCrossfade(false, true, true));
  EXPECT_FALSE(BackendOptions::SuppressSameAlbumCrossfade(true, true, false));
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
  EXPECT_EQ(50u, BackendOptions::InternalVolumeToPercent(0.5, false));
  EXPECT_EQ(0u, BackendOptions::InternalVolumeToPercent(0.0, true));
  EXPECT_EQ(100u, BackendOptions::InternalVolumeToPercent(1.0, true));
  EXPECT_EQ(80u, BackendOptions::InternalVolumeToPercent(BackendOptions::VolumeFraction(80, true), true));
  EXPECT_FALSE(BackendOptions::ShouldApplyEngineVolume(40, 40));
  EXPECT_TRUE(BackendOptions::ShouldApplyEngineVolume(40, 55));
  EXPECT_EQ(2, BackendOptions::EffectiveChannels(true, 2));
  EXPECT_EQ(0, BackendOptions::EffectiveChannels(false, 2));
  EXPECT_EQ(0, BackendOptions::EffectiveChannels(true, 0));
  EXPECT_STREQ("", BackendOptions::SoupForceHttp1(true));
  EXPECT_STREQ("1", BackendOptions::SoupForceHttp1(false));
  EXPECT_EQ(500, BackendOptions::WarmupMs(true, 500));
  EXPECT_EQ(0, BackendOptions::WarmupMs(false, 500));
}

TEST(ExitFade, DecideMatchesQtMainWindow) {
  EXPECT_EQ(ExitFade::Action::ShutdownNow, ExitFade::Decide(1, false, true, false));
  EXPECT_EQ(ExitFade::Action::ShutdownNow, ExitFade::Decide(1, true, false, false));
  EXPECT_EQ(ExitFade::Action::WaitForFade, ExitFade::Decide(1, true, true, false));
  EXPECT_TRUE(ExitFade::ShouldHideUi(ExitFade::Action::WaitForFade));
  EXPECT_TRUE(ExitFade::ShouldKeepWindow(ExitFade::Action::WaitForFade));
  EXPECT_FALSE(ExitFade::ShouldKeepWindow(ExitFade::Action::ShutdownNow));
  EXPECT_EQ(ExitFade::Action::SkipFade, ExitFade::Decide(2, true, true, false));
  EXPECT_EQ(ExitFade::Action::AbortProcess, ExitFade::Decide(2, true, true, true));
  EXPECT_TRUE(EngineFade::ShouldEmitFinished(false, false));
  EXPECT_FALSE(EngineFade::ShouldEmitFinished(true, false));
  EXPECT_FALSE(EngineFade::ShouldEmitFinished(false, true));
}

TEST(EngineFade, StopFadeGatingMatchesQt) {
  EXPECT_TRUE(EngineFade::ShouldFadeOnStop(true, false, false, false));
  EXPECT_FALSE(EngineFade::ShouldFadeOnStop(true, true, false, false));
  EXPECT_FALSE(EngineFade::ShouldFadeOnStop(true, false, true, false));
  EXPECT_FALSE(EngineFade::ShouldFadeOnStop(true, false, false, true));
  EXPECT_FALSE(EngineFade::ShouldFadeOnStop(false, false, false, false));
  EXPECT_TRUE(EngineFade::ShouldFadeOnPause(true, false));
  EXPECT_FALSE(EngineFade::ShouldFadeOnPause(true, true));
  EXPECT_FALSE(EngineFade::ShouldFadeOnPause(false, false));
  EXPECT_DOUBLE_EQ(0.5, EngineFade::VolumeAtStep(1.0, -1, 0.5));
  EXPECT_DOUBLE_EQ(0.0, EngineFade::VolumeAtStep(1.0, -1, 1.0));
  EXPECT_DOUBLE_EQ(1.0, EngineFade::VolumeAtStep(1.0, -1, 0.0));
  EXPECT_DOUBLE_EQ(0.5, EngineFade::VolumeAtStep(1.0, 1, 0.5));
  EXPECT_DOUBLE_EQ(0.0, EngineFade::VolumeAtStep(1.0, 1, 0.0));
}

TEST(EngineExclusive, BlocksSecondPipelineAndDelaysPlay) {
  EXPECT_FALSE(EngineExclusive::AllowsSecondPipeline(true));
  EXPECT_TRUE(EngineExclusive::AllowsSecondPipeline(false));
  EXPECT_FALSE(EngineExclusive::ShouldCrossfade(true, true));
  EXPECT_TRUE(EngineExclusive::ShouldCrossfade(true, false));
  EXPECT_FALSE(EngineExclusive::ShouldCrossfade(false, false));
  EXPECT_TRUE(EngineExclusive::ShouldDelayPlay(true, true));
  EXPECT_FALSE(EngineExclusive::ShouldDelayPlay(true, false));
  EXPECT_FALSE(EngineExclusive::ShouldDelayPlay(false, true));
}

TEST(EngineSeek, CoalescesRapidSeeksLikeQt) {
  EXPECT_EQ(100, EngineSeek::kDelayMs);
  EXPECT_TRUE(EngineSeek::ShouldSeekImmediately(false));
  EXPECT_FALSE(EngineSeek::ShouldSeekImmediately(true));
  EXPECT_TRUE(EngineSeek::ShouldApplyPending(true));
  EXPECT_FALSE(EngineSeek::ShouldApplyPending(false));
}

TEST(EnginePlay, ShortCircuitsAlreadyPlayingLikeQt) {
  EXPECT_TRUE(EnginePlay::IsBuffering(3));
  EXPECT_FALSE(EnginePlay::IsBuffering(-1));
  EXPECT_FALSE(EnginePlay::ShouldSeekWhenAlreadyPlaying(0, 0));
  EXPECT_TRUE(EnginePlay::ShouldSeekWhenAlreadyPlaying(1, 0));
  EXPECT_TRUE(EnginePlay::ShouldSeekWhenAlreadyPlaying(0, 5000000000ULL));
  EXPECT_TRUE(EnginePlay::ShouldShortCircuitPlayingPipeline(true, -1));
  EXPECT_FALSE(EnginePlay::ShouldShortCircuitPlayingPipeline(true, 2));
  EXPECT_FALSE(EnginePlay::ShouldShortCircuitPlayingPipeline(false, -1));
}

TEST(EngineFade, ResumeFadeInMatchesQtLatch) {
  EXPECT_TRUE(EngineFade::ShouldFadeInOnResume(true, false, false));
  EXPECT_FALSE(EngineFade::ShouldFadeInOnResume(true, false, true));
  EXPECT_TRUE(EngineFade::ShouldFadeInOnResume(false, true, false));
  EXPECT_FALSE(EngineFade::ShouldFadeInOnResume(false, false, false));
  EXPECT_TRUE(EngineFade::ShouldMarkFadedOutToPause(true));
  EXPECT_FALSE(EngineFade::ShouldMarkFadedOutToPause(false));
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

TEST(CollectionGroupingSave, AcceptsNamesAndMarksMenuChecks) {
  EXPECT_FALSE(CollectionGroupingSave::AcceptName(""));
  EXPECT_FALSE(CollectionGroupingSave::AcceptName("   "));
  EXPECT_TRUE(CollectionGroupingSave::AcceptName("Favorites"));
  CollectionGrouping::Grouping unnamed;
  EXPECT_FALSE(CollectionGroupingSave::Save("", unnamed));
  EXPECT_FALSE(CollectionGroupingSave::Save("   ", unnamed));
  EXPECT_EQ("Favorites", CollectionGroupingSave::TrimmedName("  Favorites  "));
  EXPECT_STREQ("Grouping Name", CollectionGroupingSave::DialogTitle());
  EXPECT_STREQ("Grouping name:", CollectionGroupingSave::DialogPrompt());
  EXPECT_STREQ("Save current grouping", CollectionGroupingSave::SaveLabel());
  EXPECT_STREQ("Manage saved groupings", CollectionGroupingSave::ManageLabel());
  EXPECT_EQ("✓ Group by Album", CollectionGroupingSave::MenuLabel("Group by Album", true));
  EXPECT_EQ("Group by Album", CollectionGroupingSave::MenuLabel("Group by Album", false));

  const std::vector<CollectionFilterMenu::Preset> presets = CollectionFilterMenu::BuiltinPresets();
  CollectionGrouping::Grouping album_disc;
  album_disc.first = CollectionGrouping::GroupBy::AlbumArtist;
  album_disc.second = CollectionGrouping::GroupBy::AlbumDisc;
  album_disc.third = CollectionGrouping::GroupBy::None;
  EXPECT_EQ(1, CollectionGroupingSave::MenuCheckIndex(album_disc, presets, {}));
  EXPECT_EQ("p1", CollectionGroupingSave::MenuStateKey(1, static_cast<int>(presets.size())));

  CollectionGrouping::Grouping custom;
  custom.first = CollectionGrouping::GroupBy::Year;
  custom.second = CollectionGrouping::GroupBy::Composer;
  custom.third = CollectionGrouping::GroupBy::None;
  EXPECT_EQ(-1, CollectionGroupingSave::MenuCheckIndex(custom, presets, {}));
  EXPECT_EQ("advanced", CollectionGroupingSave::MenuStateKey(-1, static_cast<int>(presets.size())));

  std::vector<std::pair<std::string, CollectionGrouping::Grouping>> saved = {{"Year / Composer", custom}};
  EXPECT_EQ(static_cast<int>(presets.size()), CollectionGroupingSave::MenuCheckIndex(custom, presets, saved));
  EXPECT_EQ("s0", CollectionGroupingSave::MenuStateKey(static_cast<int>(presets.size()), static_cast<int>(presets.size())));
}

TEST(CollectionFilterChoices, AgeRatingAndShowModeMatchQt) {
  EXPECT_EQ(6, CollectionFilterChoices::kAgeCount);
  EXPECT_EQ(6, CollectionFilterChoices::kRatingCount);
  EXPECT_EQ(3, CollectionFilterChoices::kModeCount);
  EXPECT_STREQ("Entire collection", CollectionFilterChoices::kAgeLabels[0]);
  EXPECT_STREQ("Added this week", CollectionFilterChoices::kAgeLabels[2]);
  EXPECT_STREQ("Added within three months", CollectionFilterChoices::kAgeLabels[4]);
  EXPECT_STREQ("Rating non null", CollectionFilterChoices::kRatingLabels[1]);
  EXPECT_STREQ("Rating greater than 4 stars", CollectionFilterChoices::kRatingLabels[5]);
  EXPECT_STREQ("Show all songs", CollectionFilterChoices::kModeLabels[0]);
  EXPECT_STREQ("Show only duplicates", CollectionFilterChoices::kModeLabels[1]);
  EXPECT_STREQ("Show only untagged", CollectionFilterChoices::kModeLabels[2]);
  EXPECT_STREQ("Filter by age", CollectionFilterChoices::AgeMenuTitle());
  EXPECT_STREQ("Filter by rating", CollectionFilterChoices::RatingMenuTitle());
  EXPECT_EQ(60 * 60 * 24, CollectionFilterChoices::kAgeSeconds[1]);
  EXPECT_EQ(60 * 60 * 24 * 30 * 3, CollectionFilterChoices::kAgeSeconds[4]);
  EXPECT_FLOAT_EQ(0.0f, CollectionFilterChoices::kRatingValues[1]);

  CollectionFilterOptions any = CollectionFilterChoices::FromIndices(0, 0, 0);
  EXPECT_EQ(-1, any.max_age());
  EXPECT_FLOAT_EQ(-1.0f, any.min_rating());
  EXPECT_EQ(CollectionFilterOptions::FilterMode::All, any.filter_mode());

  CollectionFilterOptions today = CollectionFilterChoices::FromIndices(1, 0, 0);
  EXPECT_EQ(86400, today.max_age());
  CollectionFilterOptions rated = CollectionFilterChoices::FromIndices(0, 1, 1);
  EXPECT_EQ(-1, rated.max_age());
  EXPECT_FLOAT_EQ(0.0f, rated.min_rating());
  EXPECT_EQ(CollectionFilterOptions::FilterMode::Duplicates, rated.filter_mode());
  Song rated_song = MakeSong("Glory Box", "Portishead", "Dummy");
  rated_song.set_rating(0.2f);
  Song unrated_song = MakeSong("Mysterons", "Portishead", "Dummy");
  unrated_song.set_url("file:///tmp/music/mysterons.flac");
  EXPECT_TRUE(rated.Matches(rated_song));
  EXPECT_FALSE(rated.Matches(unrated_song));

  CollectionFilterOptions year = CollectionFilterChoices::FromIndices(5, 5, 2);
  EXPECT_EQ(60 * 60 * 24 * 365, year.max_age());
  EXPECT_FLOAT_EQ(0.8f, year.min_rating());
  EXPECT_EQ(CollectionFilterOptions::FilterMode::Untagged, year.filter_mode());
  EXPECT_EQ(0, CollectionFilterChoices::ClampIndex(-4, CollectionFilterChoices::kAgeCount));
  EXPECT_EQ(5, CollectionFilterChoices::ClampIndex(99, CollectionFilterChoices::kAgeCount));
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
  EXPECT_TRUE(CollectionMenu::DisplayOptionsEnabled());
  EXPECT_STREQ("Display options", CollectionMenu::DisplayOptionsLabel());
}

TEST(CollectionMenu, EditTrackVsTracksAndOrganize) {
  Song one(Song::Source::Collection);
  one.set_valid(true);
  one.set_url("file:///music/a.flac");
  one.set_filetype(Song::FileType::FLAC);
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
  editable.set_filetype(Song::FileType::FLAC);
  Song stream(Song::Source::Tidal);
  stream.set_valid(true);
  stream.set_url("tidal://track/1");
  const auto mixed = CollectionMenu::VisibleItems(CollectionMenu::Analyze({editable, stream}, true, false));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::Organize));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::CopyToDevice));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::Rescan));
  EXPECT_FALSE(CollectionMenu::Contains(mixed, CollectionMenu::Action::EditTrack));
  EXPECT_TRUE(CollectionMenu::Contains(mixed, CollectionMenu::Action::EditTracks));
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

TEST(CollectionFilterKeyboard, ReturnActivatesAndEscapeFocusesFilter) {
  EXPECT_EQ(CollectionFilterKeyboard::Action::Activate, CollectionFilterKeyboard::FromSearchKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(CollectionFilterKeyboard::Action::MoveDown, CollectionFilterKeyboard::FromSearchKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(CollectionFilterKeyboard::Action::MoveUp, CollectionFilterKeyboard::FromSearchKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(CollectionFilterKeyboard::Action::Clear, CollectionFilterKeyboard::FromSearchKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(CollectionFilterKeyboard::Action::FocusFilter, CollectionFilterKeyboard::FromTreeKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(CollectionFilterKeyboard::Action::FocusFilter, CollectionFilterKeyboard::FromTreeKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(CollectionKeyboard::Action::MoveDown, CollectionFilterKeyboard::SearchMoveAction(CollectionFilterKeyboard::Action::MoveDown));
  CollectionItem root(CollectionItem::Type::Root);
  CollectionItem *divider = root.AddChild(CollectionItem::Type::Divider);
  CollectionItem *artist = root.AddChild(CollectionItem::Type::Container);
  artist->key = "Portishead";
  EXPECT_FALSE(CollectionFilterKeyboard::CanActivate(divider));
  EXPECT_TRUE(CollectionFilterKeyboard::CanActivate(artist));
  EXPECT_EQ(artist, CollectionFilterKeyboard::FirstActivatable(&root));
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

TEST(CollectionBackend, DeleteSongsBySourceRemovesOnlyThatSource) {
  const std::string path = "/tmp/strawberry-collection-source-test-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);

  Song local = MakeSong("Roads", "Portishead", "Dummy");
  local.set_directory_id(directory);
  ASSERT_GT(backend.AddOrUpdateSong(local), 0);

  Song remote = MakeSong("Helplessness Blues", "Fleet Foxes", "Helplessness Blues");
  remote.set_source(Song::Source::Subsonic);
  remote.set_url("subsonic://42");
  remote.set_directory_id(directory);
  ASSERT_GT(backend.AddOrUpdateSong(remote), 0);

  int deleted = 0;
  backend.SongsDeleted.Connect([&](const SongList &songs) { deleted = static_cast<int>(songs.size()); });
  EXPECT_EQ(1, SubsonicSettingsActions::DeleteCachedSongs(&backend));
  EXPECT_EQ(1, deleted);
  const SongList remaining = backend.Songs();
  ASSERT_EQ(1u, remaining.size());
  EXPECT_EQ(Song::Source::Collection, remaining.front().source());
  EXPECT_EQ("Roads", remaining.front().title());
  EXPECT_EQ(0, backend.DeleteSongsBySource(Song::Source::Subsonic));
  unlink(path.c_str());
}

TEST(CollectionSearchLabels, UsesQtPlaceholder) {
  EXPECT_STREQ("Enter search terms here", CollectionSearchLabels::Placeholder());
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

TEST(SkipCountEligibility, MatchesQtPercentageAndTimeMatrix) {
  const int64_t four_min = 240LL * SkipCountEligibility::kNsecPerSec;
  EXPECT_FALSE(SkipCountEligibility::ShouldIncrement(four_min, four_min));
  EXPECT_FALSE(SkipCountEligibility::ShouldIncrement(four_min - 4LL * SkipCountEligibility::kNsecPerSec, four_min));
  EXPECT_TRUE(SkipCountEligibility::ShouldIncrement(60LL * SkipCountEligibility::kNsecPerSec, four_min));
  EXPECT_FALSE(SkipCountEligibility::ShouldIncrement(0, 0));
  const int64_t twenty_one_min = 1260LL * SkipCountEligibility::kNsecPerSec;
  EXPECT_TRUE(SkipCountEligibility::ShouldIncrement(static_cast<int64_t>(0.96 * twenty_one_min), twenty_one_min));
  EXPECT_FALSE(SkipCountEligibility::ShouldIncrement(static_cast<int64_t>(0.96 * four_min), four_min));
}

TEST(CollectionSongPatch, ReplacesMatchingIds) {
  Song a;
  a.set_id(1);
  a.set_playcount(1);
  Song b;
  b.set_id(2);
  b.set_playcount(0);
  SongList songs = {a, b};
  Song updated;
  updated.set_id(1);
  updated.set_playcount(4);
  EXPECT_TRUE(CollectionSongPatch::PatchById(&songs, updated));
  EXPECT_EQ(4u, songs.front().playcount());
  EXPECT_EQ(1, CollectionSongPatch::PatchAll(&songs, {updated}));
}

TEST(CollectionBackend, EmitsStatisticsAndRatingAfterUpdate) {
  const std::string path = "/tmp/strawberry-collection-stats-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song song = MakeSong("Roads", "Portishead", "Dummy");
  song.set_directory_id(directory);
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);

  int stats = 0;
  int ratings = 0;
  backend.SongsStatisticsChanged.Connect([&](const SongList &songs) {
    ++stats;
    ASSERT_FALSE(songs.empty());
    EXPECT_EQ(id, songs.front().id());
  });
  backend.SongsRatingChanged.Connect([&](const SongList &songs) {
    ++ratings;
    ASSERT_FALSE(songs.empty());
    EXPECT_EQ(id, songs.front().id());
  });
  backend.IncrementPlayCount(id);
  backend.IncrementSkipCount(id);
  backend.SetRating(id, 0.8f);
  EXPECT_EQ(2, stats);
  EXPECT_EQ(1, ratings);
  unlink(path.c_str());
}

TEST(CollectionChangeNotify, EmitsChangedForExistingAndDiscoveredForNew) {
  EXPECT_TRUE(CollectionChangeNotify::ShouldEmitChanged(true));
  EXPECT_FALSE(CollectionChangeNotify::ShouldEmitChanged(false));
  EXPECT_TRUE(CollectionChangeNotify::ShouldEmitDiscovered(false));
  EXPECT_FALSE(CollectionChangeNotify::ShouldEmitDiscovered(true));
}

TEST(CollectionBackend, EmitsSongsChangedOnMetadataUpdate) {
  const std::string path = "/tmp/strawberry-collection-changed-test-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song song = MakeSong("Roads", "Portishead", "Dummy");
  song.set_directory_id(directory);
  int discovered = 0;
  int changed = 0;
  backend.SongsDiscovered.Connect([&](const SongList &songs) {
    ++discovered;
    ASSERT_FALSE(songs.empty());
  });
  backend.SongsChanged.Connect([&](const SongList &songs) {
    ++changed;
    ASSERT_FALSE(songs.empty());
    EXPECT_EQ("Glory Box", songs.front().title());
  });
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);
  EXPECT_EQ(1, discovered);
  EXPECT_EQ(0, changed);
  song.set_id(id);
  song.set_title("Glory Box");
  EXPECT_EQ(id, backend.AddOrUpdateSong(song));
  EXPECT_EQ(1, discovered);
  EXPECT_EQ(1, changed);
  unlink(path.c_str());
}

TEST(PlayCountIncrement, CollectionSongsOnlyAtTrackEnd) {
  Song collection(Song::Source::Collection);
  collection.set_id(8);
  EXPECT_TRUE(PlayCountIncrement::ShouldIncrementOnTrackEnd(collection));
  Song short_collection(Song::Source::Collection);
  short_collection.set_id(9);
  short_collection.set_length_nanosec(20LL * 1000000000LL);
  EXPECT_TRUE(PlayCountIncrement::ShouldIncrementOnTrackEnd(short_collection));
  Song local(Song::Source::LocalFile);
  local.set_id(10);
  EXPECT_FALSE(PlayCountIncrement::ShouldIncrementOnTrackEnd(local));
  Song radio(Song::Source::SomaFM);
  radio.set_id(11);
  EXPECT_FALSE(PlayCountIncrement::ShouldIncrementOnTrackEnd(radio));
  Song missing_id(Song::Source::Collection);
  EXPECT_FALSE(PlayCountIncrement::ShouldIncrementOnTrackEnd(missing_id));
}

TEST(CollectionAlbumArt, PropagatesManualArtAcrossAlbum) {
  EXPECT_TRUE(CollectionAlbumArt::ShouldPropagate(Song::Source::Collection));
  EXPECT_FALSE(CollectionAlbumArt::ShouldPropagate(Song::Source::SomaFM));
  const std::string path = "/tmp/strawberry-collection-albumart-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song first = MakeSong("Roads", "Portishead", "Dummy");
  first.set_directory_id(directory);
  first.set_art_manual("file:///tmp/old.jpg");
  Song second = MakeSong("Wandering Star", "Portishead", "Dummy");
  second.set_directory_id(directory);
  second.set_url("file:///tmp/music/wandering.flac");
  ASSERT_GT(backend.AddOrUpdateSong(first), 0);
  ASSERT_GT(backend.AddOrUpdateSong(second), 0);
  EXPECT_EQ(2, backend.UpdateManualAlbumArt("Portishead", "Dummy", "file:///tmp/cover.jpg"));
  const SongList album = backend.GetAlbumSongs("Portishead", "Dummy");
  ASSERT_EQ(2u, album.size());
  EXPECT_EQ("file:///tmp/cover.jpg", album[0].art_manual());
  EXPECT_EQ("file:///tmp/cover.jpg", album[1].art_manual());
  EXPECT_FALSE(album[0].art_unset());
  EXPECT_EQ(2, backend.UnsetAlbumArt("Portishead", "Dummy"));
  const SongList unset = backend.GetAlbumSongs("Portishead", "Dummy");
  ASSERT_EQ(2u, unset.size());
  EXPECT_TRUE(unset[0].art_unset());
  EXPECT_TRUE(unset[0].art_manual().empty());
  EXPECT_EQ(2, backend.ClearAlbumArt("Portishead", "Dummy", false));
  EXPECT_FALSE(backend.GetAlbumSongs("Portishead", "Dummy").front().art_unset());
  unlink(path.c_str());
}

TEST(CollectionScanProgress, PercentWithMax) {
  EXPECT_EQ(0, CollectionScanProgress::Percent(10, 0));
  EXPECT_EQ(50, CollectionScanProgress::Percent(50, 100));
  EXPECT_EQ(100, CollectionScanProgress::Percent(200, 100));
  EXPECT_TRUE(CollectionScanProgress::ShouldReport(25));
  EXPECT_FALSE(CollectionScanProgress::ShouldReport(24));
  EXPECT_EQ(3, CollectionScanProgress::Total(3));
  EXPECT_EQ(0, CollectionScanProgress::Total(-1));
  EXPECT_EQ(2, CollectionScanProgress::CountAudioPaths({"/tmp/a.flac", "/tmp/notes.txt", "/tmp/b.mp3"}));
}

TEST(CollectionScanDelay, UsesTwoSecondRescanAndDailyPeriod) {
  EXPECT_EQ(2000, CollectionScanDelay::kRescanMs);
  EXPECT_EQ(86400 * 1000, CollectionScanDelay::kPeriodicMs);
  EXPECT_TRUE(CollectionScanDelay::ShouldArm(false, false));
  EXPECT_FALSE(CollectionScanDelay::ShouldArm(true, false));
  EXPECT_FALSE(CollectionScanDelay::ShouldArm(false, true));
  EXPECT_FALSE(CollectionScanDelay::ShouldArm(false, false, true));
  EXPECT_TRUE(CollectionScanDelay::ShouldRunAfterFinish(true));
}

TEST(CollectionScanGates, SkipsIncrementalWhileTaskBlocks) {
  TaskManager manager;
  bool paused = false;
  bool resumed = false;
  manager.PauseCollectionWatchers.Connect([&paused]() { paused = true; });
  manager.ResumeCollectionWatchers.Connect([&resumed]() { resumed = true; });
  EXPECT_FALSE(CollectionScanGates::ShouldSkipIncremental(&manager));
  EXPECT_FALSE(CollectionScanGates::ShouldSkipIncremental(nullptr));
  const int id = manager.StartTask("Organizing files");
  EXPECT_FALSE(manager.GetTasks().front().blocks_collection_scans);
  manager.SetTaskBlocksCollectionScans(id);
  EXPECT_TRUE(paused);
  EXPECT_TRUE(manager.GetTasks().front().blocks_collection_scans);
  EXPECT_TRUE(CollectionScanGates::AnyBlocksCollectionScans(manager.GetTasks()));
  EXPECT_TRUE(CollectionScanGates::ShouldSkipIncremental(&manager));
  EXPECT_TRUE(CollectionScanGates::ShouldResumeWatchers(true, false));
  EXPECT_FALSE(CollectionScanGates::ShouldResumeWatchers(true, true));
  EXPECT_FALSE(CollectionScanGates::ShouldResumeWatchers(false, false));
  const int id2 = manager.StartTask("Copying files");
  manager.SetTaskBlocksCollectionScans(id2);
  manager.SetTaskFinished(id);
  EXPECT_FALSE(resumed);
  manager.SetTaskFinished(id2);
  EXPECT_TRUE(resumed);
  EXPECT_FALSE(CollectionScanGates::ShouldSkipIncremental(&manager));
}

TEST(CollectionTagSave, DefersOggAndMpegWhilePlaying) {
  Song ogg(Song::Source::Collection);
  ogg.set_url("file:///tmp/music/a.ogg");
  ogg.set_filetype(Song::FileType::OggVorbis);
  EXPECT_TRUE(CollectionTagSave::FiletypeNeedsDefer(Song::FileType::OggVorbis));
  EXPECT_TRUE(CollectionTagSave::FiletypeNeedsDefer(Song::FileType::MPEG));
  EXPECT_FALSE(CollectionTagSave::FiletypeNeedsDefer(Song::FileType::FLAC));
  EXPECT_TRUE(CollectionTagSave::ShouldDefer(ogg, "file:///tmp/music/a.ogg"));
  EXPECT_FALSE(CollectionTagSave::ShouldDefer(ogg, "file:///tmp/music/other.ogg"));
  Song flac(Song::Source::Collection);
  flac.set_url("file:///tmp/music/a.flac");
  flac.set_filetype(Song::FileType::FLAC);
  EXPECT_FALSE(CollectionTagSave::ShouldDefer(flac, "file:///tmp/music/a.flac"));
  std::map<std::string, CollectionTagSave::Pending> pending;
  ogg.set_playcount(4);
  CollectionTagSave::Queue(&pending, ogg, true, false);
  ogg.set_rating(0.8f);
  CollectionTagSave::Queue(&pending, ogg, false, true);
  ASSERT_EQ(1u, pending.size());
  EXPECT_TRUE(pending.begin()->second.save_playcount);
  EXPECT_TRUE(pending.begin()->second.save_rating);
  EXPECT_EQ(4u, pending.begin()->second.song.playcount());
  EXPECT_FLOAT_EQ(0.8f, pending.begin()->second.song.rating());
  EXPECT_TRUE(CollectionTagSave::ReadyToFlush(pending, "file:///tmp/music/a.ogg").empty());
  EXPECT_EQ(1u, CollectionTagSave::ReadyToFlush(pending, "file:///tmp/music/b.ogg").size());
}

TEST(CollectionExpire, CutoffAndShouldExpire) {
  EXPECT_EQ(0, CollectionExpire::Cutoff(1000, 0));
  EXPECT_EQ(1000 - 2 * 86400, CollectionExpire::Cutoff(1000, 2));
  EXPECT_TRUE(CollectionExpire::ShouldExpire(10, 20, true, false));
  EXPECT_FALSE(CollectionExpire::ShouldExpire(10, 20, true, true));
  EXPECT_FALSE(CollectionExpire::ShouldExpire(0, 20, true, false));
  EXPECT_FALSE(CollectionExpire::ShouldExpire(10, 20, false, false));
}

TEST(CollectionDirectoryArt, PrefersFrontThenCoverThenLargest) {
  EXPECT_TRUE(CollectionDirectoryArt::IsImageFile("/tmp/front.jpg"));
  EXPECT_FALSE(CollectionDirectoryArt::IsImageFile("/tmp/notes.txt"));
  const std::vector<std::string> filters = CollectionDirectoryArt::ParseFilters("front, cover");
  ASSERT_EQ(2u, filters.size());
  EXPECT_EQ("front", filters[0]);
  EXPECT_TRUE(CollectionDirectoryArt::NameMatches("/tmp/Front.jpg", "front"));
  EXPECT_EQ("/tmp/front.jpg", CollectionDirectoryArt::PickBestArt({"/tmp/back.jpg", "/tmp/front.jpg"}, filters));
  EXPECT_EQ("/tmp/cover.png", CollectionDirectoryArt::PickBestArt({"/tmp/cover.png", "/tmp/other.jpg"}, filters));
}

TEST(CollectionBackend, PersistsFingerprintCueLoudnessAndUserData) {
  const std::string path = "/tmp/strawberry-collection-analysis-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song song = MakeSong("Roads", "Portishead", "Dummy");
  song.set_directory_id(directory);
  song.set_fingerprint("chromaprint");
  song.set_cue_path("/tmp/music/dummy.cue");
  song.set_beginning_nanosec(1000000000);
  song.set_skipcount(4);
  song.set_lastplayed(99);
  song.set_art_manual("file:///tmp/cover.jpg");
  song.set_ebur128_integrated_loudness_lufs(-14.5);
  song.set_ebur128_loudness_range_lu(8.0);
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);
  const Song loaded = backend.SongByUrl(song.url(), song.beginning_nanosec());
  EXPECT_EQ("chromaprint", loaded.fingerprint());
  EXPECT_EQ("/tmp/music/dummy.cue", loaded.cue_path());
  EXPECT_EQ(1000000000, loaded.beginning_nanosec());
  EXPECT_EQ(4u, loaded.skipcount());
  EXPECT_EQ(99, loaded.lastplayed());
  EXPECT_EQ("file:///tmp/cover.jpg", loaded.art_manual());
  ASSERT_TRUE(loaded.ebur128_integrated_loudness_lufs());
  EXPECT_NEAR(-14.5, *loaded.ebur128_integrated_loudness_lufs(), 0.01);
  ASSERT_TRUE(loaded.ebur128_loudness_range_lu());
  EXPECT_NEAR(8.0, *loaded.ebur128_loudness_range_lu(), 0.01);

  Song extra = song;
  extra.set_beginning_nanosec(2000000000);
  extra.set_title("Wandering Star");
  extra.set_url(song.url());
  ASSERT_GT(backend.AddOrUpdateSong(extra), 0);
  EXPECT_EQ(1, backend.RetainBeginnings(song.url(), {1000000000}));
  EXPECT_EQ(id, backend.SongByUrl(song.url(), 1000000000).id());
  EXPECT_FALSE(backend.SongByUrl(song.url(), 2000000000).is_valid());
  unlink(path.c_str());
}

TEST(CollectionFingerprintMatch, RejectsEmptyAndKeepsGonePaths) {
  EXPECT_FALSE(CollectionFingerprintMatch::IsUsable(""));
  EXPECT_FALSE(CollectionFingerprintMatch::IsUsable("NONE"));
  EXPECT_TRUE(CollectionFingerprintMatch::IsUsable("abc"));
  EXPECT_TRUE(CollectionFingerprintMatch::OldPathGone("file:///tmp/strawberry-fp-missing.flac"));
  Song gone;
  gone.set_valid(true);
  gone.set_url("file:///tmp/strawberry-fp-missing.flac");
  Song same;
  same.set_valid(true);
  same.set_url("file:///tmp/strawberry-fp-new.flac");
  const Song match = CollectionFingerprintMatch::PickMovedMatch({gone, same}, "file:///tmp/strawberry-fp-new.flac");
  EXPECT_TRUE(match.is_valid());
  EXPECT_EQ(gone.url(), match.url());
  EXPECT_FALSE(CollectionFingerprintMatch::PickMovedMatch({same}, "file:///tmp/strawberry-fp-new.flac").is_valid());
}

TEST(CollectionArtPersist, UnsetClearsAutomatic) {
  Song existing;
  existing.set_art_unset(true);
  existing.set_art_automatic("file:///old.jpg");
  EXPECT_FALSE(CollectionArtPersist::ShouldApplyAutomaticArt(existing, "file:///new.jpg"));
  EXPECT_TRUE(CollectionArtPersist::ArtAutomaticForUpdate(existing, "file:///new.jpg").empty());
  existing.set_art_unset(false);
  existing.set_art_automatic("");
  EXPECT_TRUE(CollectionArtPersist::ShouldApplyAutomaticArt(existing, "file:///new.jpg"));
  EXPECT_EQ("file:///new.jpg", CollectionArtPersist::ArtAutomaticForUpdate(existing, "file:///new.jpg"));
  existing.set_art_automatic("file:///kept.jpg");
  EXPECT_EQ("file:///kept.jpg", CollectionArtPersist::ArtAutomaticForUpdate(existing, ""));
}

TEST(CollectionUnavailableRestore, RestoresUnchangedUnavailable) {
  Song existing;
  existing.set_valid(true);
  existing.set_unavailable(true);
  existing.set_mtime(100);
  existing.set_filesize(50);
  EXPECT_TRUE(CollectionUnavailableRestore::CanRestoreWithoutRescan(existing, 100, 50, false, false,
                                                                    CollectionCueScan::Change::None));
  existing.set_unavailable(false);
  EXPECT_FALSE(CollectionUnavailableRestore::CanRestoreWithoutRescan(existing, 100, 50, false, false,
                                                                     CollectionCueScan::Change::None));
  existing.set_unavailable(true);
  EXPECT_FALSE(CollectionUnavailableRestore::CanRestoreWithoutRescan(existing, 200, 50, false, false,
                                                                     CollectionCueScan::Change::None));
}

TEST(CollectionBackend, MatchesMovedFileByFingerprint) {
  const std::string path = "/tmp/strawberry-collection-fp-move-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song original = MakeSong("Moved", "Artist", "Album");
  original.set_directory_id(directory);
  original.set_url("file:///tmp/strawberry-fp-old-missing.flac");
  original.set_fingerprint("fp-moved");
  const int id = backend.AddOrUpdateSong(original);
  ASSERT_GT(id, 0);
  ASSERT_EQ(1u, backend.SongsByFingerprint("fp-moved").size());
  EXPECT_TRUE(backend.SongsByFingerprint("NONE").empty());

  Song moved = MakeSong("Moved Again", "Artist", "Album");
  moved.set_directory_id(directory);
  moved.set_url("file:///tmp/strawberry-fp-new.flac");
  moved.set_fingerprint("fp-moved");
  EXPECT_EQ(id, backend.AddOrUpdateSong(moved));
  EXPECT_EQ(id, backend.SongByUrl("file:///tmp/strawberry-fp-new.flac").id());
  EXPECT_FALSE(backend.SongByUrl("file:///tmp/strawberry-fp-old-missing.flac").is_valid());
  EXPECT_EQ("Moved Again", backend.SongById(id).title());
  unlink(path.c_str());
}

TEST(CollectionBackend, KeepsCompilationFlagsAndUnsetArt) {
  const std::string path = "/tmp/strawberry-collection-flags-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song song = MakeSong("Comp", "Artist", "Album");
  song.set_directory_id(directory);
  song.set_art_automatic("file:///tmp/front.jpg");
  const int id = backend.AddOrUpdateSong(song);
  ASSERT_GT(id, 0);
  ASSERT_EQ(1, backend.ForceCompilation({backend.SongById(id)}, true));
  EXPECT_TRUE(backend.SongById(id).compilation_on());

  Song rescan = MakeSong("Comp Rescan", "Artist", "Album");
  rescan.set_directory_id(directory);
  rescan.set_url(song.url());
  rescan.set_art_automatic("file:///tmp/other.jpg");
  EXPECT_EQ(id, backend.AddOrUpdateSong(rescan));
  const Song after_comp = backend.SongById(id);
  EXPECT_EQ("Comp Rescan", after_comp.title());
  EXPECT_TRUE(after_comp.compilation_on());
  EXPECT_FALSE(after_comp.compilation_off());

  SqlQuery unset(&db, "UPDATE songs SET art_unset = 1, art_automatic = 'file:///tmp/old.jpg' WHERE ROWID = ?");
  unset.Bind(1, id);
  ASSERT_TRUE(unset.Exec());
  EXPECT_TRUE(backend.SongById(id).art_unset());

  Song art_update = backend.SongById(id);
  art_update.set_art_unset(false);
  art_update.set_art_automatic("file:///tmp/new.jpg");
  EXPECT_EQ(id, backend.AddOrUpdateSong(art_update));
  const Song kept = backend.SongById(id);
  EXPECT_TRUE(kept.art_unset());
  EXPECT_TRUE(kept.art_automatic().empty());
  unlink(path.c_str());
}

TEST(CollectionBackend, UpdatesLastSeenAndExpiresOldUnavailable) {
  const std::string path = "/tmp/strawberry-collection-expire-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  Song keep = MakeSong("Keep", "A", "Album");
  keep.set_directory_id(directory);
  keep.set_url("file:///tmp/music/keep.flac");
  keep.set_art_automatic("file:///tmp/music/front.jpg");
  const int keep_id = backend.AddOrUpdateSong(keep);
  Song gone = MakeSong("Gone", "A", "Album");
  gone.set_directory_id(directory);
  gone.set_url("file:///tmp/music/gone.flac");
  const int gone_id = backend.AddOrUpdateSong(gone);
  backend.UpdateLastSeen(directory);
  EXPECT_GT(backend.SongById(keep_id).lastseen(), 0);
  EXPECT_EQ("file:///tmp/music/front.jpg", backend.SongById(keep_id).art_automatic());
  SqlQuery stale(&db, "UPDATE songs SET unavailable = 1, lastseen = 1 WHERE ROWID = ?");
  stale.Bind(1, gone_id);
  stale.Exec();
  EXPECT_EQ(1, backend.ExpireSongs(directory, 1, 10 * 86400));
  EXPECT_TRUE(backend.SongById(keep_id).is_valid());
  EXPECT_FALSE(backend.SongById(gone_id).is_valid());
  unlink(path.c_str());
}

TEST(CollectionFullRescan, MatchesQtRevisions) {
  EXPECT_FALSE(CollectionFullRescan::ShouldPrompt(0, 23));
  EXPECT_FALSE(CollectionFullRescan::ShouldPrompt(23, 23));
  EXPECT_TRUE(CollectionFullRescan::ShouldPrompt(20, 23));
  EXPECT_STREQ("Support for sort tags artist, album, album artist, title, composer and performer", CollectionFullRescan::ReasonFor(21));
  EXPECT_STREQ("", CollectionFullRescan::ReasonFor(22));
  EXPECT_STREQ("", CollectionFullRescan::ReasonFor(20));
  const std::vector<std::string> reasons = CollectionFullRescan::Reasons(20, 23);
  ASSERT_EQ(1u, reasons.size());
  EXPECT_EQ(CollectionFullRescan::ReasonFor(21), reasons.front());
  EXPECT_TRUE(CollectionFullRescan::Reasons(22, 23).empty());
  const std::string message = CollectionFullRescan::DialogMessage(reasons);
  EXPECT_NE(std::string::npos, message.find("full collection rescan"));
  EXPECT_NE(std::string::npos, message.find(reasons.front()));
  EXPECT_STREQ("Collection rescan notice", CollectionFullRescan::DialogTitle());
}

TEST(Database, RecordsStartupSchemaVersion) {
  const std::string path = "/tmp/strawberry-schema-startup-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  EXPECT_EQ(0, db.startup_schema_version());
  EXPECT_EQ(Database::kCurrentSchemaVersion, db.current_schema_version());
  db.Close();
  Database again(path);
  ASSERT_TRUE(again.Open());
  EXPECT_EQ(Database::kCurrentSchemaVersion, again.startup_schema_version());
  EXPECT_EQ(again.startup_schema_version(), again.current_schema_version());
  unlink(path.c_str());
}
