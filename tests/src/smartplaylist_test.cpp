#include "collection/collectionbackend.h"
#include "core/database.h"
#include "playlist/playlist.h"
#include "smartplaylists/playlistgenerator.h"
#include "smartplaylists/playlistgeneratorinserter.h"
#include "smartplaylists/playlistquerygenerator.h"
#include "smartplaylists/smartplaylist.h"
#include "smartplaylists/smartplaylistsummary.h"
#include "smartplaylists/smartplaylistpreviewpolicy.h"
#include "smartplaylists/smartplaylistsearchtermwidgetoverlay.h"
#include "smartplaylists/smartplaylisttermrow.h"
#include "smartplaylists/smartplaylisttermvalue.h"
#include "smartplaylists/smartplaylisttagcompleter.h"
#include "smartplaylists/smartplaylistquerywizardpluginsearchpage.h"
#include "smartplaylists/smartplaylistwizardfinishpage.h"
#include "smartplaylists/smartplaylistwizardlabels.h"
#include "smartplaylists/smartplaylistwizardplugin.h"
#include "smartplaylists/smartplaylistdrag.h"
#include "smartplaylists/smartplaylistgeneratemore.h"
#include "smartplaylists/smartplaylistsmodel.h"

#include <algorithm>
#include <ctime>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(SmartPlaylist, ArtistContains) {
  Song match;
  match.set_artist("Fleet Foxes");
  match.set_title("Ragged Wood");
  match.set_valid(true);
  Song other;
  other.set_artist("The Shins");
  other.set_title("New Slang");
  other.set_valid(true);

  SmartPlaylistTerm term;
  term.field = SmartPlaylistField::Artist;
  term.op = SmartPlaylistOp::Contains;
  term.value = "fleet";
  SmartPlaylistSearch search;
  search.terms.push_back(term);
  const SongList result = search.Search({match, other});
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("Ragged Wood", result.front().title());
}

TEST(SmartPlaylist, EmptyAndNotEmpty) {
  Song with_comment;
  with_comment.set_title("Has comment");
  with_comment.set_comment("live");
  Song empty_comment;
  empty_comment.set_title("No comment");
  SmartPlaylistTerm empty;
  empty.field = SmartPlaylistField::Comment;
  empty.op = SmartPlaylistOp::Empty;
  SmartPlaylistSearch search;
  search.terms.push_back(empty);
  const SongList missing = search.Search({with_comment, empty_comment});
  ASSERT_EQ(1u, missing.size());
  EXPECT_EQ("No comment", missing.front().title());

  empty.op = SmartPlaylistOp::NotEmpty;
  search.terms = {empty};
  const SongList present = search.Search({with_comment, empty_comment});
  ASSERT_EQ(1u, present.size());
  EXPECT_EQ("Has comment", present.front().title());
}

TEST(SmartPlaylist, AndOrTermsAndSort) {
  Song a;
  a.set_title("Alpha");
  a.set_artist("Fleet Foxes");
  a.set_year(2008);
  Song b;
  b.set_title("Beta");
  b.set_artist("Fleet Foxes");
  b.set_year(2011);
  Song c;
  c.set_title("Gamma");
  c.set_artist("The Shins");
  c.set_year(2001);

  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::And;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  search.terms.push_back({SmartPlaylistField::Year, SmartPlaylistOp::GreaterThan, "2009"});
  search.sort_field = SmartPlaylistField::Title;
  const SongList and_result = search.Search({a, b, c});
  ASSERT_EQ(1u, and_result.size());
  EXPECT_EQ("Beta", and_result.front().title());

  search.type = SmartPlaylistSearch::SearchType::Or;
  search.sort_field = SmartPlaylistField::Year;
  search.sort_descending = true;
  const SongList or_result = search.Search({a, b, c});
  ASSERT_EQ(2u, or_result.size());
  EXPECT_EQ("Beta", or_result.front().title());
  EXPECT_EQ("Alpha", or_result.back().title());
}

TEST(SmartPlaylist, ExtraFieldsAndLimit) {
  Song keyed;
  keyed.set_title("Key");
  keyed.set_initial_key("4A");
  keyed.set_track(2);
  keyed.set_originalyear(1999);
  Song other;
  other.set_title("Other");
  other.set_track(9);
  other.set_originalyear(2010);

  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::InitialKey, SmartPlaylistOp::Equals, "4A"});
  search.terms.push_back({SmartPlaylistField::OriginalYear, SmartPlaylistOp::LessThan, "2005"});
  const SongList result = search.Search({keyed, other});
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ("Key", result.front().title());

  search.terms.clear();
  search.sort_field = SmartPlaylistField::Track;
  search.sort_descending = true;
  search.limit = 1;
  const SongList limited = search.Search({keyed, other});
  ASSERT_EQ(1u, limited.size());
  EXPECT_EQ("Other", limited.front().title());
}

TEST(SmartPlaylist, SerializeRoundTrip) {
  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::Or;
  search.limit = 25;
  search.sort_field = SmartPlaylistField::Year;
  search.sort_descending = true;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  search.terms.push_back({SmartPlaylistField::Comment, SmartPlaylistOp::Empty, {}});
  const std::string blob = search.Serialize();
  SmartPlaylistSearch parsed;
  ASSERT_TRUE(SmartPlaylistSearch::Parse(blob, &parsed));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::Or, parsed.type);
  EXPECT_EQ(25, parsed.limit);
  EXPECT_EQ(SmartPlaylistField::Year, parsed.sort_field);
  EXPECT_TRUE(parsed.sort_descending);
  EXPECT_FALSE(parsed.sort_random);
  ASSERT_EQ(2u, parsed.terms.size());
  EXPECT_EQ(SmartPlaylistField::Artist, parsed.terms[0].field);
  EXPECT_EQ(SmartPlaylistOp::Contains, parsed.terms[0].op);
  EXPECT_EQ("fleet", parsed.terms[0].value);
  EXPECT_EQ(SmartPlaylistOp::Empty, parsed.terms[1].op);

  SmartPlaylistSearch::AddSaved("Fleet years", search);
  const auto saved = SmartPlaylistSearch::LoadSaved();
  bool found = false;
  for (const auto &preset : saved) {
    if (preset.first == "Fleet years") {
      found = true;
      EXPECT_EQ(25, preset.second.limit);
    }
  }
  EXPECT_TRUE(found);
  SmartPlaylistSearch parsed_saved;
  ASSERT_TRUE(SmartPlaylistSearch::FindSaved("Fleet years", &parsed_saved));
  EXPECT_EQ(25, parsed_saved.limit);
  SmartPlaylistSearch updated = search;
  updated.limit = 10;
  SmartPlaylistSearch::RenameSaved("Fleet years", "Fleet years renamed", updated);
  EXPECT_FALSE(SmartPlaylistSearch::FindSaved("Fleet years", &parsed_saved));
  ASSERT_TRUE(SmartPlaylistSearch::FindSaved("Fleet years renamed", &parsed_saved));
  EXPECT_EQ(10, parsed_saved.limit);
  SmartPlaylistSearch::RemoveSaved("Fleet years renamed");
  EXPECT_FALSE(SmartPlaylistSearch::FindSaved("Fleet years renamed", &parsed_saved));
}

TEST(SmartPlaylistSummary, DescribesTermsLimitAndEmpty) {
  EXPECT_STREQ("Dynamic mode is on", SmartPlaylistSummary::Title());
  EXPECT_STREQ("New tracks will be added automatically.", SmartPlaylistSummary::EmptyTerms());
  SmartPlaylistSearch empty;
  EXPECT_EQ(SmartPlaylistSummary::EmptyTerms(), SmartPlaylistSummary::Summary(empty));

  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "Portishead"});
  search.terms.push_back({SmartPlaylistField::Year, SmartPlaylistOp::GreaterThan, "1990"});
  search.limit = 50;
  EXPECT_EQ("Artist Contains Portishead and Year Greater than 1990 · limit 50", SmartPlaylistSummary::Summary(search));

  search.type = SmartPlaylistSearch::SearchType::Or;
  search.limit = 0;
  EXPECT_EQ("Artist Contains Portishead or Year Greater than 1990", SmartPlaylistSummary::Summary(search));

  SmartPlaylistSearch all_songs;
  all_songs.type = SmartPlaylistSearch::SearchType::All;
  all_songs.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "ignored"});
  all_songs.limit = 12;
  EXPECT_EQ("Include all songs · limit 12", SmartPlaylistSummary::Summary(all_songs));
  EXPECT_EQ("3 songs will be added as “Favorites”. Include all songs · limit 12",
            SmartPlaylistSummary::FinishText(3, "Favorites", all_songs));

  SmartPlaylistTerm empty_comment;
  empty_comment.field = SmartPlaylistField::Comment;
  empty_comment.op = SmartPlaylistOp::Empty;
  EXPECT_EQ("Comment Empty", SmartPlaylistSummary::TermText(empty_comment));
}

TEST(SmartPlaylist, FieldAndOpIndex) {
  EXPECT_EQ(SmartPlaylistField::Title, SmartPlaylistSearch::FieldFromIndex(0));
  EXPECT_EQ(SmartPlaylistField::InitialKey, SmartPlaylistSearch::FieldFromIndex(static_cast<int>(SmartPlaylistSearch::FieldNames().size()) - 1));
  EXPECT_EQ(SmartPlaylistOp::Contains, SmartPlaylistSearch::OpFromIndex(0));
  EXPECT_EQ(SmartPlaylistOp::RelativeDate, SmartPlaylistSearch::OpFromIndex(static_cast<int>(SmartPlaylistSearch::OpNames().size()) - 1));
  EXPECT_EQ(SmartPlaylistSearch::FieldNames().size(), 29u);
  EXPECT_EQ(SmartPlaylistSearch::OpNames().size(), 12u);
  EXPECT_EQ(SmartPlaylistFieldKind::Date, SmartPlaylistSearch::KindOf(SmartPlaylistField::LastPlayed));
  EXPECT_EQ(SmartPlaylistFieldKind::Number, SmartPlaylistSearch::KindOf(SmartPlaylistField::Year));
  EXPECT_EQ(SmartPlaylistFieldKind::Text, SmartPlaylistSearch::KindOf(SmartPlaylistField::Artist));
  const auto date_ops = SmartPlaylistSearch::OperatorsFor(SmartPlaylistField::DateCreated);
  EXPECT_NE(date_ops.end(), std::find(date_ops.begin(), date_ops.end(), SmartPlaylistOp::NumericDate));
  EXPECT_NE(date_ops.end(), std::find(date_ops.begin(), date_ops.end(), SmartPlaylistOp::RelativeDate));
}

TEST(SmartPlaylist, DateAndLengthOperators) {
  const int64_t day = SmartPlaylistSearch::ParseDateValue("2020-06-15");
  EXPECT_GT(day, 0);
  Song on_day;
  on_day.set_valid(true);
  on_day.set_title("Roads");
  on_day.set_ctime(day + 3600);
  SmartPlaylistTerm numeric;
  numeric.field = SmartPlaylistField::DateCreated;
  numeric.op = SmartPlaylistOp::NumericDate;
  numeric.value = "2020-06-15";
  EXPECT_TRUE(numeric.Matches(on_day));
  numeric.value = "2020-06-16";
  EXPECT_FALSE(numeric.Matches(on_day));

  Song recent;
  recent.set_valid(true);
  recent.set_title("Helplessness Blues");
  recent.set_lastplayed(static_cast<int64_t>(std::time(nullptr)) - 2 * 86400);
  SmartPlaylistTerm relative;
  relative.field = SmartPlaylistField::LastPlayed;
  relative.op = SmartPlaylistOp::RelativeDate;
  relative.value = "7";
  EXPECT_TRUE(relative.Matches(recent));
  relative.value = "1";
  EXPECT_FALSE(relative.Matches(recent));

  Song long_song;
  long_song.set_valid(true);
  long_song.set_title("The Trial");
  long_song.set_length_nanosec(300LL * 1000000000LL);
  SmartPlaylistTerm length;
  length.field = SmartPlaylistField::Length;
  length.op = SmartPlaylistOp::GreaterThan;
  length.value = "120";
  EXPECT_TRUE(length.Matches(long_song));
  length.value = "400";
  EXPECT_FALSE(length.Matches(long_song));
}

TEST(PlaylistGenerator, CreateQueryRoundTripAndInsert) {
  auto generator = PlaylistGenerator::Create(PlaylistGenerator::Type::Query);
  ASSERT_TRUE(generator);
  EXPECT_EQ(PlaylistGenerator::Type::Query, generator->type());
  EXPECT_FALSE(generator->is_dynamic());

  SmartPlaylistSearch search;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  search.limit = 50;
  generator->Load(search.Serialize());
  EXPECT_EQ(search.Serialize(), generator->Save());

  auto query = std::dynamic_pointer_cast<PlaylistQueryGenerator>(generator);
  ASSERT_TRUE(query);
  query->set_dynamic(true);
  EXPECT_TRUE(query->is_dynamic());
  EXPECT_EQ(50, query->GetDynamicFuture());
  EXPECT_TRUE(query->Generate().empty());

  const std::string path = "/tmp/strawberry-smartgen-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);
  Song match;
  match.set_title("Ragged Wood");
  match.set_artist("Fleet Foxes");
  match.set_url("file:///tmp/music/ragged.flac");
  match.set_directory_id(directory);
  match.set_valid(true);
  ASSERT_GT(backend.AddOrUpdateSong(match), 0);
  Song other;
  other.set_title("New Slang");
  other.set_artist("The Shins");
  other.set_url("file:///tmp/music/slang.flac");
  other.set_directory_id(directory);
  other.set_valid(true);
  ASSERT_GT(backend.AddOrUpdateSong(other), 0);

  query->set_collection_backend(&backend);
  const SongList generated = query->Generate();
  ASSERT_EQ(1u, generated.size());
  EXPECT_EQ("Ragged Wood", generated.front().title());
  EXPECT_TRUE(query->GenerateMore(1).empty());

  Playlist playlist;
  PlaylistGeneratorInserter inserter;
  EXPECT_EQ(1, inserter.Insert(&playlist, query));
  EXPECT_EQ(1, playlist.row_count());
  unlink(path.c_str());
}

TEST(SmartPlaylistGenerateMore, TrimHistoryKeepsNewest) {
  EXPECT_EQ(std::vector<int>({3, 4}), SmartPlaylistGenerateMore::TrimHistory({1, 2, 3, 4}, 2));
  EXPECT_EQ(std::vector<int>({1, 2}), SmartPlaylistGenerateMore::TrimHistory({1, 2}, 5));
  EXPECT_TRUE(SmartPlaylistGenerateMore::Prepare(SmartPlaylistSearch{}, {7}, 3, 1).id_not_in == std::vector<int>({7}));
}

TEST(PlaylistGenerator, GenerateMorePagesByTitle) {
  const std::string path = "/tmp/strawberry-smartgen-more-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);
  for (const char *title : {"Alpha", "Beta", "Gamma"}) {
    Song song;
    song.set_title(title);
    song.set_artist("Fleet Foxes");
    song.set_url(std::string("file:///tmp/music/") + title + ".flac");
    song.set_directory_id(directory);
    song.set_valid(true);
    ASSERT_GT(backend.AddOrUpdateSong(song), 0);
  }

  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::All;
  search.limit = 1;
  search.sort_field = SmartPlaylistField::Title;
  auto query = std::make_shared<PlaylistQueryGenerator>("Pages", search, true);
  query->set_collection_backend(&backend);
  const SongList first = query->Generate();
  ASSERT_EQ(1u, first.size());
  EXPECT_EQ("Alpha", first.front().title());
  const SongList second = query->GenerateMore(1);
  ASSERT_EQ(1u, second.size());
  EXPECT_EQ("Beta", second.front().title());
  const SongList third = query->GenerateMore(1);
  ASSERT_EQ(1u, third.size());
  EXPECT_EQ("Gamma", third.front().title());
  EXPECT_TRUE(query->GenerateMore(1).empty());
  unlink(path.c_str());
}

TEST(PlaylistGenerator, GenerateMoreExcludesByIdNotUrl) {
  const std::string path = "/tmp/strawberry-smartgen-ids-" + std::to_string(getpid()) + ".db";
  unlink(path.c_str());
  Database db(path);
  ASSERT_TRUE(db.Open());
  CollectionBackend backend(&db);
  const int directory = backend.AddDirectory("/tmp/music");
  ASSERT_GE(directory, 0);
  Song first;
  first.set_title("Alpha Cue");
  first.set_url("file:///tmp/music/album.flac");
  first.set_beginning_nanosec(0);
  first.set_directory_id(directory);
  first.set_valid(true);
  const int first_id = backend.AddOrUpdateSong(first);
  ASSERT_GT(first_id, 0);
  Song second;
  second.set_title("Beta Cue");
  second.set_url("file:///tmp/music/album.flac");
  second.set_beginning_nanosec(1000000000);
  second.set_directory_id(directory);
  second.set_valid(true);
  const int second_id = backend.AddOrUpdateSong(second);
  ASSERT_GT(second_id, 0);
  EXPECT_NE(first_id, second_id);

  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::All;
  search.limit = 1;
  search.sort_field = SmartPlaylistField::Title;
  auto query = std::make_shared<PlaylistQueryGenerator>("Cue", search, true);
  query->set_collection_backend(&backend);
  const SongList page1 = query->Generate();
  ASSERT_EQ(1u, page1.size());
  EXPECT_EQ(first_id, page1.front().id());
  const SongList page2 = query->GenerateMore(1);
  ASSERT_EQ(1u, page2.size());
  EXPECT_EQ(second_id, page2.front().id());
  EXPECT_EQ("file:///tmp/music/album.flac", page2.front().url());
  unlink(path.c_str());
}

TEST(SmartPlaylistsModel, BuiltinSavedAndWizardKeys) {
  SmartPlaylistSearch custom;
  custom.terms.push_back({SmartPlaylistField::Title, SmartPlaylistOp::Contains, "live"});
  SmartPlaylistSearch::AddSaved("Live tracks", custom);
  SmartPlaylistsModel model;
  model.Reload();
  EXPECT_TRUE(model.ItemByKey("all"));
  EXPECT_TRUE(model.ItemByKey("never"));
  EXPECT_TRUE(model.ItemByKey("rated"));
  EXPECT_TRUE(model.ItemByKey("newest"));
  EXPECT_TRUE(model.ItemByKey("played"));
  ASSERT_TRUE(model.ItemByKey("saved:Live tracks"));
  EXPECT_EQ(SmartPlaylistsItem::Kind::Saved, model.ItemByKey("saved:Live tracks")->kind);
  ASSERT_TRUE(model.ItemByKey("wizard"));
  EXPECT_EQ(SmartPlaylistsItem::Kind::Wizard, model.ItemByKey("wizard")->kind);
  EXPECT_FALSE(model.ItemByKey("missing"));
  SmartPlaylistSearch::RemoveSaved("Live tracks");

  SmartPlaylistSearch leftover;
  leftover.terms.push_back({SmartPlaylistField::Title, SmartPlaylistOp::Contains, "tmp"});
  SmartPlaylistSearch::AddSaved("Temporary", leftover);
  SmartPlaylistsModel restored;
  restored.Reload();
  ASSERT_TRUE(restored.ItemByKey("saved:Temporary"));
  restored.RestoreDefaults();
  EXPECT_FALSE(restored.ItemByKey("saved:Temporary"));
  EXPECT_TRUE(restored.ItemByKey("all"));
  EXPECT_TRUE(SmartPlaylistSearch::LoadSaved().empty());

  SmartPlaylistQueryWizardPlugin plugin(custom);
  auto generated = plugin.CreateGenerator("From wizard", true);
  ASSERT_TRUE(generated);
  EXPECT_EQ("From wizard", generated->name());
  EXPECT_TRUE(generated->is_dynamic());
}

TEST(SmartPlaylist, AllIgnoresTermsAndRandomRoundTrips) {
  EXPECT_STREQ("And", SmartPlaylistSearch::TypeName(SmartPlaylistSearch::SearchType::And));
  EXPECT_STREQ("Or", SmartPlaylistSearch::TypeName(SmartPlaylistSearch::SearchType::Or));
  EXPECT_STREQ("All", SmartPlaylistSearch::TypeName(SmartPlaylistSearch::SearchType::All));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::And, SmartPlaylistSearch::TypeFromName("And"));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::Or, SmartPlaylistSearch::TypeFromName("Or"));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::All, SmartPlaylistSearch::TypeFromName("All"));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::And, SmartPlaylistSearch::TypeFromName("unknown"));
  EXPECT_TRUE(SmartPlaylistSearch::TermsApply(SmartPlaylistSearch::SearchType::And));
  EXPECT_TRUE(SmartPlaylistSearch::TermsApply(SmartPlaylistSearch::SearchType::Or));
  EXPECT_FALSE(SmartPlaylistSearch::TermsApply(SmartPlaylistSearch::SearchType::All));
  EXPECT_FALSE(SmartPlaylistSearch().IsValid());
  SmartPlaylistSearch all_type;
  all_type.type = SmartPlaylistSearch::SearchType::All;
  EXPECT_TRUE(all_type.IsValid());

  Song a;
  a.set_title("Alpha");
  a.set_artist("Fleet Foxes");
  Song b;
  b.set_title("Beta");
  b.set_artist("The Shins");
  SmartPlaylistSearch search;
  search.type = SmartPlaylistSearch::SearchType::All;
  search.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  const SongList all = search.Search({a, b});
  ASSERT_EQ(2u, all.size());

  search.sort_random = true;
  search.limit = 1;
  const SongList limited = search.Search({a, b});
  ASSERT_EQ(1u, limited.size());
  EXPECT_TRUE(limited.front().title() == "Alpha" || limited.front().title() == "Beta");

  search.limit = 25;
  search.sort_field = SmartPlaylistField::Year;
  search.sort_descending = true;
  const std::string blob = search.Serialize();
  EXPECT_NE(std::string::npos, blob.find("All,"));
  EXPECT_NE(std::string::npos, blob.find(",Random"));
  SmartPlaylistSearch parsed;
  ASSERT_TRUE(SmartPlaylistSearch::Parse(blob, &parsed));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::All, parsed.type);
  EXPECT_TRUE(parsed.sort_random);
  EXPECT_EQ(SmartPlaylistSearch::SortType::Random, parsed.sort_type());
  EXPECT_EQ(25, parsed.limit);
  EXPECT_TRUE(parsed.sort_descending);
  EXPECT_EQ(1u, parsed.terms.size());

  SmartPlaylistSearch legacy;
  ASSERT_TRUE(SmartPlaylistSearch::Parse("And,0,0,0;0,0,fleet", &legacy));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::And, legacy.type);
  EXPECT_FALSE(legacy.sort_random);
  EXPECT_EQ(SmartPlaylistSearch::SortType::FieldAsc, legacy.sort_type());
  ASSERT_EQ(1u, legacy.terms.size());
  EXPECT_EQ("fleet", legacy.terms[0].value);

  SmartPlaylistSearch field_desc;
  field_desc.set_sort_type(SmartPlaylistSearch::SortType::FieldDesc);
  EXPECT_FALSE(field_desc.sort_random);
  EXPECT_TRUE(field_desc.sort_descending);
  EXPECT_EQ(SmartPlaylistSearch::SortType::FieldDesc, field_desc.sort_type());
}

TEST(SmartPlaylistWizardLabels, SearchSortLimitAndDynamicCopy) {
  EXPECT_STREQ("Search mode", SmartPlaylistWizardLabels::SearchMode());
  EXPECT_STREQ("Match every search term (AND)", SmartPlaylistWizardLabels::And());
  EXPECT_STREQ("Match one or more search terms (OR)", SmartPlaylistWizardLabels::Or());
  EXPECT_STREQ("Include all songs", SmartPlaylistWizardLabels::All());
  EXPECT_STREQ("Search terms", SmartPlaylistWizardLabels::SearchTerms());
  EXPECT_STREQ("Sorting", SmartPlaylistWizardLabels::Sorting());
  EXPECT_STREQ("Put songs in a random order", SmartPlaylistWizardLabels::Random());
  EXPECT_STREQ("Sort songs by", SmartPlaylistWizardLabels::SortBy());
  EXPECT_STREQ("Limits", SmartPlaylistWizardLabels::Limits());
  EXPECT_STREQ("Show all the songs", SmartPlaylistWizardLabels::ShowAll());
  EXPECT_STREQ("Only show the first", SmartPlaylistWizardLabels::OnlyFirst());
  EXPECT_STREQ(" songs", SmartPlaylistWizardLabels::Songs());
  EXPECT_STREQ("Name", SmartPlaylistWizardLabels::Name());
  EXPECT_STREQ("Use dynamic mode", SmartPlaylistWizardLabels::UseDynamic());
  EXPECT_STREQ("In dynamic mode new tracks will be chosen and added to the playlist every time a song finishes.",
              SmartPlaylistWizardLabels::DynamicHint());
  EXPECT_EQ(0, SmartPlaylistWizardLabels::TypeIndex(SmartPlaylistSearch::SearchType::And));
  EXPECT_EQ(1, SmartPlaylistWizardLabels::TypeIndex(SmartPlaylistSearch::SearchType::Or));
  EXPECT_EQ(2, SmartPlaylistWizardLabels::TypeIndex(SmartPlaylistSearch::SearchType::All));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::And, SmartPlaylistWizardLabels::TypeFromIndex(0));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::Or, SmartPlaylistWizardLabels::TypeFromIndex(1));
  EXPECT_EQ(SmartPlaylistSearch::SearchType::All, SmartPlaylistWizardLabels::TypeFromIndex(2));
  EXPECT_TRUE(SmartPlaylistWizardLabels::TermsSensitive(SmartPlaylistSearch::SearchType::And));
  EXPECT_FALSE(SmartPlaylistWizardLabels::TermsSensitive(SmartPlaylistSearch::SearchType::All));
  EXPECT_TRUE(SmartPlaylistWizardLabels::ShowAllSongs(0));
  EXPECT_TRUE(SmartPlaylistWizardLabels::ShowAllSongs(-1));
  EXPECT_FALSE(SmartPlaylistWizardLabels::ShowAllSongs(15));
  EXPECT_EQ(0, SmartPlaylistWizardLabels::LimitFromUi(true, 15));
  EXPECT_EQ(15, SmartPlaylistWizardLabels::LimitFromUi(false, 15));
  EXPECT_EQ(0, SmartPlaylistWizardLabels::LimitFromUi(false, 0));
  EXPECT_EQ(15, SmartPlaylistWizardLabels::LimitSpinOrDefault(0));
  EXPECT_EQ(40, SmartPlaylistWizardLabels::LimitSpinOrDefault(40));
  EXPECT_FALSE(SmartPlaylistWizardLabels::FieldSortSensitive(true));
  EXPECT_TRUE(SmartPlaylistWizardLabels::FieldSortSensitive(false));
  EXPECT_FALSE(SmartPlaylistWizardLabels::LimitSpinSensitive(true));
  EXPECT_TRUE(SmartPlaylistWizardLabels::LimitSpinSensitive(false));
  EXPECT_EQ(3u, SmartPlaylistWizardLabels::SearchTypeChoices().size());
}

TEST(SmartPlaylistTermRow, PlaceholderAndRemoveMatchQt) {
  EXPECT_STREQ("Add search term", SmartPlaylistTermRow::OverlayLabel());
  EXPECT_STREQ("Add search term", SmartPlaylistSearchTermWidgetOverlay::Label());
  EXPECT_TRUE(SmartPlaylistTermRow::RowSensitive(true));
  EXPECT_FALSE(SmartPlaylistTermRow::RowSensitive(false));
  EXPECT_TRUE(SmartPlaylistTermRow::ShowsRemove(true));
  EXPECT_FALSE(SmartPlaylistTermRow::ShowsRemove(false));
  EXPECT_EQ(1, SmartPlaylistTermRow::InitialActiveTerms(false, 0));
  EXPECT_EQ(1, SmartPlaylistTermRow::InitialActiveTerms(false, 5));
  EXPECT_EQ(2, SmartPlaylistTermRow::InitialActiveTerms(true, 2));
  EXPECT_EQ(0, SmartPlaylistTermRow::InitialActiveTerms(true, 0));
  EXPECT_TRUE(SmartPlaylistTermRow::KeepsPlaceholder());
}

TEST(SmartPlaylistTermValue, EditorsAndDateTimeMatchQt) {
  using SmartPlaylistTermValue::Editor;
  EXPECT_EQ(Editor::Empty, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Text, SmartPlaylistOp::Empty));
  EXPECT_EQ(Editor::Empty, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Rating, SmartPlaylistOp::NotEmpty));
  EXPECT_EQ(Editor::RelativeDays, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Date, SmartPlaylistOp::RelativeDate));
  EXPECT_EQ(Editor::Calendar, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Date, SmartPlaylistOp::NumericDate));
  EXPECT_EQ(Editor::Calendar, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Date, SmartPlaylistOp::GreaterThan));
  EXPECT_EQ(Editor::Rating, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Rating, SmartPlaylistOp::Equals));
  EXPECT_EQ(Editor::Time, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Time, SmartPlaylistOp::LessThan));
  EXPECT_EQ(Editor::Number, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Number, SmartPlaylistOp::GreaterThan));
  EXPECT_EQ(Editor::Text, SmartPlaylistTermValue::EditorFor(SmartPlaylistFieldKind::Text, SmartPlaylistOp::Contains));

  EXPECT_EQ("2020-03-05", SmartPlaylistTermValue::FormatDate(2020, 3, 5));
  int year = 0;
  int month = 0;
  int day = 0;
  EXPECT_TRUE(SmartPlaylistTermValue::ParseDate("2020-03-05", &year, &month, &day));
  EXPECT_EQ(2020, year);
  EXPECT_EQ(3, month);
  EXPECT_EQ(5, day);
  EXPECT_FALSE(SmartPlaylistTermValue::ParseDate("not-a-date", &year, &month, &day));
  EXPECT_FALSE(SmartPlaylistTermValue::ParseDate("1899-01-01", &year, &month, &day));

  EXPECT_EQ(125, SmartPlaylistTermValue::TimeToSeconds(0, 2, 5));
  EXPECT_EQ(3723, SmartPlaylistTermValue::TimeToSeconds(1, 2, 3));
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  SmartPlaylistTermValue::SecondsToTime(3723, &hours, &minutes, &seconds);
  EXPECT_EQ(1, hours);
  EXPECT_EQ(2, minutes);
  EXPECT_EQ(3, seconds);
  EXPECT_EQ("0.50", SmartPlaylistTermValue::FormatRating(0.5f));
  EXPECT_EQ("-1.00", SmartPlaylistTermValue::FormatRating(-1.0f));

  Song timed;
  timed.set_length_nanosec(125LL * 1000000000);
  timed.set_valid(true);
  SmartPlaylistTerm length;
  length.field = SmartPlaylistField::Length;
  length.op = SmartPlaylistOp::Equals;
  length.value = std::to_string(SmartPlaylistTermValue::TimeToSeconds(0, 2, 5));
  EXPECT_TRUE(length.Matches(timed));

  Song dated;
  dated.set_ctime(SmartPlaylistSearch::ParseDateValue("2020-03-05"));
  dated.set_valid(true);
  SmartPlaylistTerm on_date;
  on_date.field = SmartPlaylistField::DateCreated;
  on_date.op = SmartPlaylistOp::NumericDate;
  on_date.value = SmartPlaylistTermValue::FormatDate(2020, 3, 5);
  EXPECT_TRUE(on_date.Matches(dated));

  Song rated;
  rated.set_rating(0.5f);
  rated.set_valid(true);
  SmartPlaylistTerm rating;
  rating.field = SmartPlaylistField::Rating;
  rating.op = SmartPlaylistOp::Equals;
  rating.value = SmartPlaylistTermValue::FormatRating(0.5f);
  EXPECT_TRUE(rating.Matches(rated));
}

TEST(SmartPlaylistTagCompleter, CompletesQtArtistAlbumFields) {
  EXPECT_TRUE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Artist));
  EXPECT_TRUE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Album));
  EXPECT_TRUE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::AlbumArtist));
  EXPECT_FALSE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Title));
  EXPECT_FALSE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Genre));
  EXPECT_FALSE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Composer));
  EXPECT_FALSE(SmartPlaylistTagCompleter::CompletesField(SmartPlaylistField::Year));
  EXPECT_EQ(PlaylistColumn::Artist, SmartPlaylistTagCompleter::ColumnFor(SmartPlaylistField::Artist));
  EXPECT_EQ(PlaylistColumn::Album, SmartPlaylistTagCompleter::ColumnFor(SmartPlaylistField::Album));
  EXPECT_EQ(PlaylistColumn::AlbumArtist, SmartPlaylistTagCompleter::ColumnFor(SmartPlaylistField::AlbumArtist));
  EXPECT_TRUE(SmartPlaylistTagCompleter::ShouldAttach(SmartPlaylistTermValue::Editor::Text, SmartPlaylistField::Artist));
  EXPECT_FALSE(SmartPlaylistTagCompleter::ShouldAttach(SmartPlaylistTermValue::Editor::Number, SmartPlaylistField::Artist));
  EXPECT_FALSE(SmartPlaylistTagCompleter::ShouldAttach(SmartPlaylistTermValue::Editor::Text, SmartPlaylistField::Title));

  Song radiohead;
  radiohead.set_artist("Radiohead");
  radiohead.set_album("OK Computer");
  radiohead.set_albumartist("Radiohead");
  radiohead.set_valid(true);
  Song portishead;
  portishead.set_artist("Portishead");
  portishead.set_album("Dummy");
  portishead.set_albumartist("Portishead");
  portishead.set_valid(true);
  const auto artists = SmartPlaylistTagCompleter::ValuesFor({radiohead, portishead}, SmartPlaylistField::Artist);
  ASSERT_EQ(2u, artists.size());
  EXPECT_EQ("Portishead", artists[0]);
  EXPECT_EQ("Radiohead", artists[1]);
  EXPECT_TRUE(SmartPlaylistTagCompleter::ValuesFor({radiohead}, SmartPlaylistField::Title).empty());
}

TEST(SmartPlaylistWizardFinishPage, IsCompleteRequiresNonEmptyName) {
  EXPECT_FALSE(SmartPlaylistWizardFinishPage::IsComplete(""));
  EXPECT_TRUE(SmartPlaylistWizardFinishPage::IsComplete("Favorites"));
  EXPECT_TRUE(SmartPlaylistWizardFinishPage::IsComplete(" "));
}

TEST(SmartPlaylistTerm, IsValidMatchesQtSearchTerm) {
  SmartPlaylistTerm empty_text;
  empty_text.field = SmartPlaylistField::Artist;
  empty_text.op = SmartPlaylistOp::Contains;
  EXPECT_FALSE(empty_text.IsValid());
  empty_text.value = "fleet";
  EXPECT_TRUE(empty_text.IsValid());
  empty_text.op = SmartPlaylistOp::Empty;
  empty_text.value.clear();
  EXPECT_TRUE(empty_text.IsValid());

  SmartPlaylistTerm rating;
  rating.field = SmartPlaylistField::Rating;
  rating.op = SmartPlaylistOp::Equals;
  rating.value = "-1.00";
  EXPECT_FALSE(rating.IsValid());
  rating.value = "0.50";
  EXPECT_TRUE(rating.IsValid());

  SmartPlaylistTerm length;
  length.field = SmartPlaylistField::Length;
  length.op = SmartPlaylistOp::Equals;
  length.value = "0";
  EXPECT_TRUE(length.IsValid());

  SmartPlaylistTerm on_date;
  on_date.field = SmartPlaylistField::DateCreated;
  on_date.op = SmartPlaylistOp::NumericDate;
  EXPECT_FALSE(on_date.IsValid());
  on_date.value = "2020-03-05";
  EXPECT_TRUE(on_date.IsValid());

  SmartPlaylistTerm year;
  year.field = SmartPlaylistField::Year;
  year.op = SmartPlaylistOp::GreaterThan;
  year.value = "0";
  EXPECT_TRUE(year.IsValid());
}

TEST(SmartPlaylistQueryWizardPluginSearchPage, IsCompleteRequiresValidTermsUnlessAll) {
  EXPECT_TRUE(SmartPlaylistQueryWizardPluginSearchPage::IsComplete(SmartPlaylistSearch::SearchType::All, {false}));
  EXPECT_TRUE(SmartPlaylistQueryWizardPluginSearchPage::IsComplete(SmartPlaylistSearch::SearchType::And, {}));
  EXPECT_TRUE(SmartPlaylistQueryWizardPluginSearchPage::IsComplete(SmartPlaylistSearch::SearchType::And, {true, true}));
  EXPECT_FALSE(SmartPlaylistQueryWizardPluginSearchPage::IsComplete(SmartPlaylistSearch::SearchType::And, {true, false}));
  EXPECT_FALSE(SmartPlaylistQueryWizardPluginSearchPage::IsComplete(SmartPlaylistSearch::SearchType::Or, {false}));
  EXPECT_TRUE(SmartPlaylistWizardFinishPage::CanCreate("Favorites", true));
  EXPECT_FALSE(SmartPlaylistWizardFinishPage::CanCreate("Favorites", false));
  EXPECT_FALSE(SmartPlaylistWizardFinishPage::CanCreate("", true));
}

TEST(SmartPlaylistPreviewPolicy, TermPreviewClearsLimitAndSkipsInvalidTerms) {
  using SmartPlaylistPreviewPolicy::Kind;
  EXPECT_TRUE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Terms, true, true));
  EXPECT_FALSE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Terms, false, true));
  EXPECT_TRUE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Terms, false, false));
  EXPECT_TRUE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Sort, true, true));
  EXPECT_FALSE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Sort, false, true));
  EXPECT_FALSE(SmartPlaylistPreviewPolicy::ShouldUpdate(Kind::Sort, false, false));
  EXPECT_EQ(0, SmartPlaylistPreviewPolicy::LimitForPreview(Kind::Terms, 15));
  EXPECT_EQ(0, SmartPlaylistPreviewPolicy::LimitForPreview(Kind::Terms, 0));
  EXPECT_EQ(15, SmartPlaylistPreviewPolicy::LimitForPreview(Kind::Sort, 15));
  EXPECT_EQ(0, SmartPlaylistPreviewPolicy::LimitForPreview(Kind::Sort, 0));

  SmartPlaylistSearch a;
  a.terms.push_back({SmartPlaylistField::Artist, SmartPlaylistOp::Contains, "fleet"});
  a.limit = 15;
  a.sort_field = SmartPlaylistField::Year;
  a.sort_descending = true;
  SmartPlaylistSearch b = a;
  EXPECT_TRUE(SmartPlaylistPreviewPolicy::SameSearch(a, b));
  b.limit = 20;
  EXPECT_FALSE(SmartPlaylistPreviewPolicy::SameSearch(a, b));
  b = a;
  b.terms[0].value = "shins";
  EXPECT_FALSE(SmartPlaylistPreviewPolicy::SameSearch(a, b));

  const SmartPlaylistSearch term_preview = SmartPlaylistPreviewPolicy::SearchForPreview(a, Kind::Terms);
  EXPECT_EQ(0, term_preview.limit);
  EXPECT_EQ(15, a.limit);
  EXPECT_EQ(1u, term_preview.terms.size());
  const SmartPlaylistSearch sort_preview = SmartPlaylistPreviewPolicy::SearchForPreview(a, Kind::Sort);
  EXPECT_EQ(15, sort_preview.limit);
}

TEST(SmartPlaylistDrag, JoinsSongUrlsAndSkipsWizard) {
  Song a;
  a.set_url("file:///a");
  Song b;
  b.set_url("file:///b");
  Song empty;
  EXPECT_EQ("file:///a\nfile:///b", SmartPlaylistDrag::DragPayload({a, b, empty}));
  EXPECT_TRUE(SmartPlaylistDrag::CanDrag(false));
  EXPECT_FALSE(SmartPlaylistDrag::CanDrag(true));
}
