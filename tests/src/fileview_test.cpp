#include "collection/collectionmodel.h"
#include "collection/collectiontree.h"
#include "device/devicecopy.h"
#include "device/devicesongmenu.h"
#include "device/devicedrag.h"
#include "device/devicekeyboard.h"
#include "fileview/fileviewdrag.h"
#include "fileview/fileviewmenu.h"
#include "fileview/fileviewhidden.h"
#include "fileview/fileviewhistory.h"
#include "fileview/fileviewkeyboard.h"
#include "fileview/fileviewsongs.h"
#include "fileview/fileviewtreemodel.h"
#include "utilities/fileutils.h"
#include "widgets/listboxkeyboard.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <set>
#include <string>

namespace {

std::string TempDir() {
  char path[] = "/tmp/strawberry-fileview-XXXXXX";
  return mkdtemp(path);
}

}  // namespace

TEST(FileViewTreeModel, RootPathsAndFilesIn) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "song.flac");
  const std::string other = FileUtils::Join(dir, "notes.txt");
  const std::string hidden = FileUtils::Join(dir, ".hidden.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  ASSERT_TRUE(FileUtils::WriteFile(other, "b"));
  ASSERT_TRUE(FileUtils::WriteFile(hidden, "c"));

  FileViewTreeModel model;
  model.SetRootPaths({dir});
  EXPECT_EQ(1, model.DirectoryCount());
  ASSERT_TRUE(model.root());
  EXPECT_EQ(FileViewTreeItem::Type::Root, model.root()->type);

  const std::vector<std::string> files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), audio), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), other), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), hidden), files.end());

  FileUtils::Remove(audio);
  FileUtils::Remove(other);
  FileUtils::Remove(hidden);
  rmdir(dir.c_str());
}

TEST(FileViewTreeModel, NameFiltersIncludeExtraExtensions) {
  const std::string dir = TempDir();
  const std::string cue = FileUtils::Join(dir, "album.cue");
  const std::string txt = FileUtils::Join(dir, "readme.txt");
  ASSERT_TRUE(FileUtils::WriteFile(cue, "PERFORMER \"A\""));
  ASSERT_TRUE(FileUtils::WriteFile(txt, "hi"));

  FileViewTreeModel model;
  model.SetNameFilters({"txt"});
  const std::vector<std::string> files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), cue), files.end());
  EXPECT_NE(std::find(files.begin(), files.end(), txt), files.end());

  FileUtils::Remove(cue);
  FileUtils::Remove(txt);
  rmdir(dir.c_str());
}

TEST(FileViewHidden, ShouldIncludeEntry) {
  EXPECT_TRUE(FileViewHidden::IsHiddenEntry(".hidden.flac"));
  EXPECT_FALSE(FileViewHidden::IsHiddenEntry("song.flac"));
  EXPECT_FALSE(FileViewHidden::ShouldIncludeEntry(".hidden.flac", false));
  EXPECT_TRUE(FileViewHidden::ShouldIncludeEntry(".hidden.flac", true));
  EXPECT_TRUE(FileViewHidden::ShouldIncludeEntry("song.flac", false));
  EXPECT_FALSE(FileViewHidden::ShouldIncludeEntry("", true));
}

TEST(FileViewTreeModel, ShowHiddenAndAllFiles) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "song.flac");
  const std::string hidden = FileUtils::Join(dir, ".hidden.flac");
  const std::string notes = FileUtils::Join(dir, "notes.txt");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  ASSERT_TRUE(FileUtils::WriteFile(hidden, "b"));
  ASSERT_TRUE(FileUtils::WriteFile(notes, "c"));

  FileViewTreeModel model;
  std::vector<std::string> files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), audio), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), hidden), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), notes), files.end());

  model.SetShowHidden(true);
  files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), hidden), files.end());
  EXPECT_EQ(std::find(files.begin(), files.end(), notes), files.end());

  model.SetShowAllFiles(true);
  files = model.FilesIn(dir);
  EXPECT_NE(std::find(files.begin(), files.end(), notes), files.end());

  FileUtils::Remove(audio);
  FileUtils::Remove(hidden);
  FileUtils::Remove(notes);
  rmdir(dir.c_str());
}

TEST(FileViewTreeModel, MissingRootIsIgnored) {
  FileViewTreeModel model;
  model.SetRootPaths({"/tmp/does-not-exist-strawberry-fileview"});
  EXPECT_EQ(0, model.DirectoryCount());
}

TEST(FileViewSongs, FromPathsSkipsDirectoriesAndReadsTags) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "x"));

  const SongList bare = FileViewSongs::FromPaths({audio, dir, ""});
  ASSERT_EQ(1u, bare.size());
  EXPECT_TRUE(bare[0].is_valid());
  EXPECT_EQ(FileUtils::UriFromPath(audio), bare[0].url());
  EXPECT_EQ("roads.flac", bare[0].title());
  EXPECT_EQ(Song::Source::LocalFile, bare[0].source());

  const SongList tagged = FileViewSongs::FromPaths({audio}, [](const std::string &path) {
    Song song(Song::Source::LocalFile);
    song.set_valid(true);
    song.set_url(FileUtils::UriFromPath(path));
    song.set_title("Roads");
    song.set_artist("Portishead");
    return song;
  });
  ASSERT_EQ(1u, tagged.size());
  EXPECT_EQ("Roads", tagged[0].title());
  EXPECT_EQ("Portishead", tagged[0].artist());

  Song invalid(Song::Source::LocalFile);
  const SongList fallback = FileViewSongs::FromPaths({audio}, [&](const std::string &) { return invalid; });
  ASSERT_EQ(1u, fallback.size());
  EXPECT_TRUE(fallback[0].is_valid());
  EXPECT_EQ("roads.flac", fallback[0].title());

  FileUtils::Remove(audio);
  rmdir(dir.c_str());
}

TEST(FileViewMenu, PlaylistActionsMatchQt) {
  EXPECT_EQ(9, FileViewMenu::ItemCount());
  EXPECT_EQ(FileViewMenu::Action::Append, FileViewMenu::FromId("append"));
  EXPECT_EQ(FileViewMenu::Action::Replace, FileViewMenu::FromId("replace"));
  EXPECT_EQ(FileViewMenu::Action::New, FileViewMenu::FromId("new"));
  EXPECT_EQ(FileViewMenu::Action::Browse, FileViewMenu::FromId("browse"));
  EXPECT_EQ(FileViewMenu::Action::Delete, FileViewMenu::FromId("delete"));
  EXPECT_TRUE(FileViewMenu::IsPlaylistAction(FileViewMenu::Action::Append));
  EXPECT_TRUE(FileViewMenu::IsPlaylistAction(FileViewMenu::Action::Replace));
  EXPECT_TRUE(FileViewMenu::IsPlaylistAction(FileViewMenu::Action::New));
  EXPECT_FALSE(FileViewMenu::IsPlaylistAction(FileViewMenu::Action::Browse));
  EXPECT_EQ("/tmp/song.flac", FileViewMenu::BrowserPath({"/tmp/song.flac", "/tmp/other.flac"}));
  EXPECT_TRUE(FileViewMenu::BrowserPath({}).empty());
  const CollectionBehaviour::Plan replace =
      FileViewMenu::PlanFor(FileViewMenu::Action::Replace, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_TRUE(replace.clear_current);
  const CollectionBehaviour::Plan open_new =
      FileViewMenu::PlanFor(FileViewMenu::Action::New, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, open_new.destination);

  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "x"));
  const std::string nested = FileUtils::Join(dir, "nested");
  ASSERT_EQ(0, mkdir(nested.c_str(), 0755));
  const std::vector<std::string> expanded = FileViewMenu::ExpandPaths({dir, audio});
  ASSERT_EQ(2u, expanded.size());
  EXPECT_EQ(audio, expanded[0]);
  EXPECT_EQ(audio, expanded[1]);
}

TEST(FileViewHistory, BackForwardAndTruncate) {
  FileViewHistory history;
  EXPECT_FALSE(history.CanBack());
  EXPECT_FALSE(history.CanForward());
  history.Push("/music");
  history.Push("/music/portishead");
  history.Push("/music/portishead/dummy");
  EXPECT_TRUE(history.CanBack());
  EXPECT_FALSE(history.CanForward());
  EXPECT_EQ("/music/portishead", history.Back());
  EXPECT_TRUE(history.CanForward());
  EXPECT_EQ("/music/portishead/dummy", history.Forward());
  history.Back();
  history.Push("/music/radiohead");
  EXPECT_EQ("/music/radiohead", history.Current());
  EXPECT_FALSE(history.CanForward());
  EXPECT_EQ(2, history.index());
  ASSERT_EQ(3u, history.items().size());
  EXPECT_EQ("/music/radiohead", history.items().back());
}

TEST(FileViewDrag, SkipsDirectoriesAndEmitsFileUris) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  const std::string payload = FileViewDrag::DragPayload({dir, audio, ""});
  EXPECT_EQ(FileUtils::UriFromPath(audio), payload);
  EXPECT_TRUE(payload.find('\n') == std::string::npos);
  EXPECT_TRUE(FileViewDrag::DragPayload({dir}).empty());
  FileUtils::Remove(audio);
  rmdir(dir.c_str());
}

TEST(ListBoxKeyboard, FromKeyAndWrapAround) {
  EXPECT_EQ(ListBoxKeyboard::Action::Activate, ListBoxKeyboard::FromKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(ListBoxKeyboard::Action::Activate, ListBoxKeyboard::FromKey(ListBoxKeyboard::kKPEnter));
  EXPECT_EQ(ListBoxKeyboard::Action::MoveUp, ListBoxKeyboard::FromKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(ListBoxKeyboard::Action::MoveDown, ListBoxKeyboard::FromKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(ListBoxKeyboard::Action::Home, ListBoxKeyboard::FromKey(ListBoxKeyboard::kHome));
  EXPECT_EQ(ListBoxKeyboard::Action::End, ListBoxKeyboard::FromKey(ListBoxKeyboard::kEnd));
  EXPECT_EQ(ListBoxKeyboard::Action::Escape, ListBoxKeyboard::FromKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(ListBoxKeyboard::Action::Delete, ListBoxKeyboard::FromKey(ListBoxKeyboard::kDelete));
  EXPECT_EQ(ListBoxKeyboard::Action::Backspace, ListBoxKeyboard::FromKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(ListBoxKeyboard::Action::None, ListBoxKeyboard::FromKey('a'));
  EXPECT_EQ(-1, ListBoxKeyboard::NextIndex(0, 0, ListBoxKeyboard::Action::MoveDown));
  EXPECT_EQ(0, ListBoxKeyboard::NextIndex(2, 3, ListBoxKeyboard::Action::Home));
  EXPECT_EQ(2, ListBoxKeyboard::NextIndex(0, 3, ListBoxKeyboard::Action::End));
  EXPECT_EQ(2, ListBoxKeyboard::NextIndex(0, 3, ListBoxKeyboard::Action::MoveUp));
  EXPECT_EQ(0, ListBoxKeyboard::NextIndex(2, 3, ListBoxKeyboard::Action::MoveDown));
  EXPECT_EQ(1, ListBoxKeyboard::NextIndex(-1, 3, ListBoxKeyboard::Action::MoveDown));
  EXPECT_EQ(1, ListBoxKeyboard::FirstPrefixIndex({"Roads", "Dummy", "Glory Box"}, "du"));
  EXPECT_EQ(2, ListBoxKeyboard::FirstPrefixIndex({"Roads", "Dummy", "Glory Box"}, "G"));
  EXPECT_EQ(-1, ListBoxKeyboard::FirstPrefixIndex({"Roads"}, "z"));
  EXPECT_EQ(-1, ListBoxKeyboard::FirstPrefixIndex({"Roads"}, ""));
}

TEST(FileViewKeyboard, AltAndHistoryBack) {
  EXPECT_EQ(FileViewKeyboard::Action::Activate, FileViewKeyboard::FromKey(ListBoxKeyboard::kReturn, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::FromKey(ListBoxKeyboard::kUp, true));
  EXPECT_EQ(FileViewKeyboard::Action::MoveUp, FileViewKeyboard::FromKey(ListBoxKeyboard::kUp, false));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryBack, FileViewKeyboard::FromKey(ListBoxKeyboard::kLeft, true));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryForward, FileViewKeyboard::FromKey(ListBoxKeyboard::kRight, true));
  EXPECT_EQ(FileViewKeyboard::Action::Home, FileViewKeyboard::FromKey(ListBoxKeyboard::kHome, true));
  EXPECT_EQ(FileViewKeyboard::Action::First, FileViewKeyboard::FromKey(ListBoxKeyboard::kHome, false));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryBack, FileViewKeyboard::FromKey(ListBoxKeyboard::kBackSpace, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::HistoryBack, false));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryBack, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::HistoryBack, true));
  EXPECT_EQ(FileViewKeyboard::Action::Home, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::Home, false));
}

TEST(DeviceKeyboard, FromKeyBackAndSpecialRows) {
  EXPECT_EQ(DeviceKeyboard::Action::Activate, DeviceKeyboard::FromKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(DeviceKeyboard::Action::MoveUp, DeviceKeyboard::FromKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(DeviceKeyboard::Action::MoveDown, DeviceKeyboard::FromKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(DeviceKeyboard::Action::Home, DeviceKeyboard::FromKey(ListBoxKeyboard::kHome));
  EXPECT_EQ(DeviceKeyboard::Action::End, DeviceKeyboard::FromKey(ListBoxKeyboard::kEnd));
  EXPECT_EQ(DeviceKeyboard::Action::Back, DeviceKeyboard::FromKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(DeviceKeyboard::Action::Escape, DeviceKeyboard::FromKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(DeviceKeyboard::Action::None, DeviceKeyboard::FromKey('a'));
  EXPECT_EQ(ListBoxKeyboard::Action::Home, DeviceKeyboard::MoveAction(DeviceKeyboard::Action::Home));
  EXPECT_EQ(ListBoxKeyboard::Action::None, DeviceKeyboard::MoveAction(DeviceKeyboard::Action::Back));
  EXPECT_TRUE(DeviceKeyboard::IsSpecialRowKind("back"));
  EXPECT_TRUE(DeviceKeyboard::IsSpecialRowKind("add-all"));
  EXPECT_FALSE(DeviceKeyboard::IsSpecialRowKind("song"));
  EXPECT_FALSE(DeviceKeyboard::IsSpecialRowKind(nullptr));
}

TEST(DeviceSongMenu, PlaylistActionsMatchQt) {
  EXPECT_EQ(5, DeviceSongMenu::ItemCount());
  EXPECT_EQ(DeviceSongMenu::Action::Append, DeviceSongMenu::FromId("append"));
  EXPECT_EQ(DeviceSongMenu::Action::Replace, DeviceSongMenu::FromId("replace"));
  EXPECT_EQ(DeviceSongMenu::Action::New, DeviceSongMenu::FromId("new"));
  EXPECT_EQ(DeviceSongMenu::Action::Copy, DeviceSongMenu::FromId("copy"));
  EXPECT_EQ(DeviceSongMenu::Action::Delete, DeviceSongMenu::FromId("delete"));
  EXPECT_TRUE(DeviceSongMenu::IsPlaylistAction(DeviceSongMenu::Action::Append));
  EXPECT_TRUE(DeviceSongMenu::IsPlaylistAction(DeviceSongMenu::Action::Replace));
  EXPECT_TRUE(DeviceSongMenu::IsPlaylistAction(DeviceSongMenu::Action::New));
  EXPECT_FALSE(DeviceSongMenu::IsPlaylistAction(DeviceSongMenu::Action::Copy));
  EXPECT_FALSE(DeviceSongMenu::IsPlaylistAction(DeviceSongMenu::Action::Delete));
  EXPECT_EQ(4u, DeviceSongMenu::VisibleItems({}).size());
  Song song(Song::Source::Device);
  song.set_url("gphoto2://phone/Music/roads.mp3");
  EXPECT_EQ(5u, DeviceSongMenu::VisibleItems({song}).size());
  const CollectionBehaviour::Plan append = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::Append, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_FALSE(append.clear_current);
  EXPECT_EQ(CollectionBehaviour::Destination::Current, append.destination);
  const CollectionBehaviour::Plan replace = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::Replace, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_TRUE(replace.clear_current);
  const CollectionBehaviour::Plan open_new = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::New, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, open_new.destination);
}

TEST(DeviceCopy, CollectionRequestCopiesWithoutMove) {
  EXPECT_FALSE(DeviceCopy::CanCopyToCollection({}));
  Song song(Song::Source::Device);
  song.set_url("gphoto2://phone/Music/roads.mp3");
  song.set_title("Roads");
  const OrganizeDialog::Request request = DeviceCopy::CollectionRequest({song});
  ASSERT_EQ(1u, request.songs.size());
  EXPECT_EQ("Roads", request.songs.front().title());
  EXPECT_FALSE(request.move);
  EXPECT_TRUE(request.destination.empty());
  EXPECT_TRUE(DeviceCopy::CanCopyToCollection(request.songs));
}

TEST(DeviceCollectionTree, GroupsDeviceSongsUntilExpanded) {
  Song a(Song::Source::Device);
  a.set_valid(true);
  a.set_title("Roads");
  a.set_artist("Portishead");
  a.set_albumartist("Portishead");
  a.set_album("Dummy");
  a.set_url("gphoto2://phone/Music/roads.mp3");
  Song b(Song::Source::Device);
  b.set_valid(true);
  b.set_title("Mysterons");
  b.set_artist("Portishead");
  b.set_albumartist("Portishead");
  b.set_album("Dummy");
  b.set_url("gphoto2://phone/Music/mysterons.mp3");
  CollectionGrouping::Grouping grouping;
  grouping.first = CollectionGrouping::GroupBy::AlbumArtist;
  grouping.second = CollectionGrouping::GroupBy::Album;
  grouping.third = CollectionGrouping::GroupBy::None;
  CollectionModel model;
  model.Reset({a, b}, grouping, false, false, false);
  ASSERT_TRUE(model.root());
  std::set<std::string> expanded;
  EXPECT_EQ(0, CollectionTree::VisibleSongCount(model.root(), false, expanded));
  EXPECT_EQ(2u, CollectionTree::SongsFromItem(model.root()).size());
  CollectionTree::CollectExpandableKeys(model.root(), &expanded);
  EXPECT_EQ(2, CollectionTree::VisibleSongCount(model.root(), false, expanded));
}

TEST(DeviceDrag, JoinsSongUrls) {
  Song a(Song::Source::Device);
  a.set_url("gphoto2://phone/Music/a.mp3");
  Song b(Song::Source::Device);
  b.set_url("gphoto2://phone/Music/b.mp3");
  Song empty;
  EXPECT_EQ("gphoto2://phone/Music/a.mp3\ngphoto2://phone/Music/b.mp3", DeviceDrag::DragPayload({a, b, empty}));
  EXPECT_TRUE(DeviceDrag::DragPayload({}).empty());
}
