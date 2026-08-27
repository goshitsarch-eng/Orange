#include "collection/collectionmodel.h"
#include "collection/collectiontree.h"
#include "collection/collectiontreeclick.h"
#include "device/devicepropertiesicons.h"
#include "device/devicepropertiesinfo.h"
#include "device/devicepropertieslabels.h"
#include "device/devicesupportedformats.h"
#include "device/devicecopysupported.h"
#include "organize/organizetranscode.h"
#include "core/musicstorage.h"
#include "device/devicecopy.h"
#include "device/devicecopyjob.h"
#include "device/devicecopyrunner.h"
#include "core/taskmanager.h"
#include "device/devicemenu.h"
#include "device/devicesongmenu.h"
#include "device/devicedrag.h"
#include "device/devicedeletedialog.h"
#include "device/deviceconnectdialog.h"
#include "device/deviceforgetdialog.h"
#include "device/deviceviewlook.h"
#include "device/deviceviewreload.h"
#include "device/devicecopyrefresh.h"
#include "device/devicescanprogress.h"
#include "device/devicekeyboard.h"
#include "fileview/fileviewdrag.h"
#include "fileview/fileviewicons.h"
#include "fileview/fileviewmenu.h"
#include "fileview/fileviewmode.h"
#include "fileview/fileviewnav.h"
#include "fileview/fileviewhidden.h"
#include "fileview/fileviewhistory.h"
#include "fileview/fileviewkeyboard.h"
#include "fileview/fileviewsongs.h"
#include "fileview/fileviewurls.h"
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

TEST(FileViewUrls, KeepsDirectoriesAsFileUris) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "x"));
  const std::vector<std::string> urls = FileViewUrls::FromPaths({audio, dir, ""});
  ASSERT_EQ(2u, urls.size());
  EXPECT_EQ(FileUtils::UriFromPath(audio), urls[0]);
  EXPECT_EQ(FileUtils::UriFromPath(dir), urls[1]);
  EXPECT_TRUE(FileViewUrls::FromPath({}).empty());
  FileUtils::Remove(audio);
  rmdir(dir.c_str());
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
  EXPECT_EQ("/tmp", FileViewMenu::BrowserDirectory("/tmp/song.flac"));
  const auto same_dir = FileViewMenu::BrowserPaths({"/tmp/song.flac", "/tmp/other.flac"});
  ASSERT_EQ(1u, same_dir.size());
  EXPECT_EQ("/tmp/song.flac", same_dir.front());
  const auto two_dirs = FileViewMenu::BrowserPaths({"/tmp/song.flac", "/home/other.flac"});
  ASSERT_EQ(2u, two_dirs.size());
  EXPECT_EQ("/tmp/song.flac", two_dirs.front());
  EXPECT_EQ("/home/other.flac", two_dirs.back());
  EXPECT_TRUE(FileViewMenu::BrowserPaths({}).empty());
  EXPECT_EQ(FileViewMenu::BrowserOpenPolicy::Open, FileViewMenu::BrowserPolicy(3));
  EXPECT_EQ(FileViewMenu::BrowserOpenPolicy::Open, FileViewMenu::BrowserPolicy(5));
  EXPECT_EQ(FileViewMenu::BrowserOpenPolicy::Confirm, FileViewMenu::BrowserPolicy(6));
  EXPECT_EQ(FileViewMenu::BrowserOpenPolicy::TooMany, FileViewMenu::BrowserPolicy(51));
  EXPECT_STREQ("Too many songs selected.", FileViewMenu::BrowserTooManyMessage());
  EXPECT_EQ("8 songs in 6 different directories selected, are you sure you want to open them all?",
            FileViewMenu::BrowserConfirmMessage(8, 6));
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

TEST(FileViewMenu, ExpandPathsRecursesNestedAlbums) {
  const std::string album = TempDir();
  const std::string cd1 = FileUtils::Join(album, "CD1");
  const std::string cd2 = FileUtils::Join(album, "CD2");
  ASSERT_EQ(0, mkdir(cd1.c_str(), 0755));
  ASSERT_EQ(0, mkdir(cd2.c_str(), 0755));
  const std::string a = FileUtils::Join(cd1, "a.flac");
  const std::string b = FileUtils::Join(cd2, "b.flac");
  const std::string cover = FileUtils::Join(album, "cover.jpg");
  const std::string playlist = FileUtils::Join(album, "set.m3u");
  const std::string notes = FileUtils::Join(album, "notes.txt");
  ASSERT_TRUE(FileUtils::WriteFile(a, "a"));
  ASSERT_TRUE(FileUtils::WriteFile(b, "b"));
  ASSERT_TRUE(FileUtils::WriteFile(cover, "cover"));
  ASSERT_TRUE(FileUtils::WriteFile(playlist, "#EXTM3U\n"));
  ASSERT_TRUE(FileUtils::WriteFile(notes, "notes"));

  EXPECT_TRUE(FileViewMenu::IsLoadableFile(a));
  EXPECT_TRUE(FileViewMenu::IsLoadableFile(playlist));
  EXPECT_FALSE(FileViewMenu::IsLoadableFile(cover));
  EXPECT_FALSE(FileViewMenu::IsLoadableFile(notes));

  const std::vector<std::string> collected = FileViewMenu::CollectLoadablePaths(album);
  ASSERT_EQ(3u, collected.size());
  EXPECT_NE(std::find(collected.begin(), collected.end(), a), collected.end());
  EXPECT_NE(std::find(collected.begin(), collected.end(), b), collected.end());
  EXPECT_NE(std::find(collected.begin(), collected.end(), playlist), collected.end());
  EXPECT_EQ(std::find(collected.begin(), collected.end(), cover), collected.end());
  EXPECT_EQ(std::find(collected.begin(), collected.end(), notes), collected.end());

  const std::vector<std::string> expanded = FileViewMenu::ExpandPaths({album, notes, playlist});
  ASSERT_EQ(4u, expanded.size());
  EXPECT_EQ(1, std::count(expanded.begin(), expanded.end(), a));
  EXPECT_EQ(1, std::count(expanded.begin(), expanded.end(), b));
  EXPECT_EQ(2, std::count(expanded.begin(), expanded.end(), playlist));
  EXPECT_EQ(0, std::count(expanded.begin(), expanded.end(), notes));

  const SongList songs = FileViewSongs::FromPaths(FileViewMenu::ExpandPaths({album}));
  ASSERT_EQ(3u, songs.size());

  FileUtils::Remove(a);
  FileUtils::Remove(b);
  FileUtils::Remove(cover);
  FileUtils::Remove(playlist);
  FileUtils::Remove(notes);
  rmdir(cd1.c_str());
  rmdir(cd2.c_str());
  rmdir(album.c_str());
}

TEST(FileViewTreeClick, MiddleButtonEnqueues) {
  EXPECT_EQ(CollectionTreeClick::Action::Enqueue,
            CollectionTreeClick::FromPress(CollectionTreeClick::kMiddleButton, 1, static_cast<GdkModifierType>(0)));
  EXPECT_TRUE(CollectionTreeClick::SelectRowBeforeEnqueue(false));
  EXPECT_FALSE(CollectionTreeClick::SelectRowBeforeEnqueue(true));
  const auto paths = FileViewMenu::TreeEnqueuePaths("/music/portishead/dummy.flac");
  ASSERT_EQ(1u, paths.size());
  EXPECT_EQ("/music/portishead/dummy.flac", paths.front());
  EXPECT_TRUE(FileViewMenu::TreeEnqueuePaths("").empty());
}

TEST(FileViewMenu, DoubleClickPlaylistNameMatchesQt) {
  EXPECT_TRUE(FileViewMenu::DoubleClickPlaylistName({}).empty());
  EXPECT_EQ("/music/portishead/dummy.flac", FileViewMenu::DoubleClickPlaylistName({"/music/portishead/dummy.flac"}));
  EXPECT_EQ("/music/a.flac", FileViewMenu::DoubleClickPlaylistName({"/music/a.flac", "/music/b.flac"}));
  const CollectionBehaviour::Plan load =
      CollectionBehaviour::FromDoubleClick(BehaviourSettings::AddBehaviour::Load, BehaviourSettings::PlayBehaviour::Always, false);
  EXPECT_TRUE(load.clear_current);
  EXPECT_TRUE(load.should_play);
  const CollectionBehaviour::Plan created =
      CollectionBehaviour::FromDoubleClick(BehaviourSettings::AddBehaviour::OpenInNew, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, created.destination);
  EXPECT_FALSE(created.should_play);
}

TEST(FileViewMenu, NewPlaylistNameMatchesQtFileView) {
  char short_buf[] = "/tmp/fv-XXXXXX";
  ASSERT_NE(mkdtemp(short_buf), nullptr);
  const std::string short_dir = short_buf;
  ASSERT_LE(short_dir.size(), static_cast<size_t>(FileViewMenu::kNewPlaylistNameMaxLength));
  EXPECT_EQ(short_dir, FileViewMenu::NewPlaylistName({short_dir}, "/music", false));
  EXPECT_EQ(short_dir, FileViewMenu::NewPlaylistName({short_dir}, "/music", true));

  const std::string long_dir = TempDir();
  ASSERT_GT(long_dir.size(), static_cast<size_t>(FileViewMenu::kNewPlaylistNameMaxLength));
  EXPECT_EQ(FileUtils::BaseName(long_dir), FileViewMenu::NewPlaylistName({long_dir}, "/music", false));

  EXPECT_EQ("/music", FileViewMenu::NewPlaylistName({"/tmp/song.flac"}, "/music", false));
  EXPECT_STREQ("Files", FileViewMenu::TreeDefaultPlaylistName());
  EXPECT_EQ("Files", FileViewMenu::NewPlaylistName({"/tmp/song.flac"}, "/music", true));
  EXPECT_EQ("Dummy", FileViewMenu::PathPlaylistName("/home/user/Music/Portishead/Dummy"));
  EXPECT_FALSE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::New));

  rmdir(long_dir.c_str());
  rmdir(short_dir.c_str());
}

TEST(FileViewMenu, DeleteUsesRawSelectionPaths) {
  const std::string album = "/tmp/album";
  const std::vector<std::string> selection = {album};
  EXPECT_FALSE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::Delete));
  EXPECT_FALSE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::Append));
  EXPECT_FALSE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::Replace));
  EXPECT_TRUE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::Device));
  EXPECT_TRUE(FileViewMenu::ExpandsPaths(FileViewMenu::Action::EditTags));
  EXPECT_TRUE(FileViewMenu::PathsForAction(FileViewMenu::Action::Append, selection) == selection);
  const auto deleted = FileViewMenu::PathsForAction(FileViewMenu::Action::Delete, selection);
  ASSERT_EQ(1u, deleted.size());
  EXPECT_EQ(album, deleted.front());
  EXPECT_TRUE(FileViewMenu::PathsForAction(FileViewMenu::Action::Copy, selection) == selection);
  EXPECT_TRUE(FileViewMenu::PathsForAction(FileViewMenu::Action::Browse, selection) == selection);
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

TEST(FileViewNav, UpEnabledMatchesQtCdUp) {
  EXPECT_FALSE(FileViewNav::UpEnabled(""));
  EXPECT_FALSE(FileViewNav::UpEnabled("/"));
  EXPECT_FALSE(FileViewNav::UpEnabled("///"));
  EXPECT_TRUE(FileViewNav::UpEnabled("/home"));
  EXPECT_TRUE(FileViewNav::UpEnabled("/home/user"));
  EXPECT_TRUE(FileViewNav::UpEnabled("/home/user/"));
}

TEST(FileViewDrag, PathsForDragUsesSelectionOrDraggedRow) {
  EXPECT_TRUE(FileViewDrag::PathsForDrag({"/a", "/b"}, "").empty());
  const std::vector<std::string> selected = FileViewDrag::PathsForDrag({"/a", "/b"}, "/b");
  ASSERT_EQ(2u, selected.size());
  EXPECT_EQ("/a", selected[0]);
  EXPECT_EQ("/b", selected[1]);
  const std::vector<std::string> dragged = FileViewDrag::PathsForDrag({"/a", "/b"}, "/c");
  ASSERT_EQ(1u, dragged.size());
  EXPECT_EQ("/c", dragged[0]);
}

TEST(FileViewDrag, EmitsDirectoryAndFileUris) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  const std::string payload = FileViewDrag::DragPayload({dir, audio, ""});
  EXPECT_EQ(FileUtils::UriFromPath(dir) + "\n" + FileUtils::UriFromPath(audio), payload);
  EXPECT_EQ(FileUtils::UriFromPath(dir), FileViewDrag::DragPayload({dir}));
  EXPECT_TRUE(FileViewDrag::DragPayload({}).empty());
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

TEST(FileViewMenu, KeyboardOpensContextMenu) {
  EXPECT_TRUE(FileViewMenu::IsKeyboardTrigger(FileViewMenu::kMenuKey, 0));
  EXPECT_TRUE(FileViewMenu::IsKeyboardTrigger(FileViewMenu::kF10Key, FileViewMenu::kShiftMask));
  EXPECT_FALSE(FileViewMenu::IsKeyboardTrigger(FileViewMenu::kF10Key, 0));
  EXPECT_TRUE(FileViewMenu::ListShouldShowMenu());
  EXPECT_TRUE(FileViewMenu::TreeShouldShowMenu(true));
  EXPECT_FALSE(FileViewMenu::TreeShouldShowMenu(false));
}

TEST(FileViewKeyboard, AltAndHistoryBack) {
  EXPECT_EQ(FileViewKeyboard::Action::Activate, FileViewKeyboard::FromKey(ListBoxKeyboard::kReturn, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::FromKey(ListBoxKeyboard::kUp, true));
  EXPECT_EQ(FileViewKeyboard::Action::MoveUp, FileViewKeyboard::FromKey(ListBoxKeyboard::kUp, false));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryBack, FileViewKeyboard::FromKey(ListBoxKeyboard::kLeft, true));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryForward, FileViewKeyboard::FromKey(ListBoxKeyboard::kRight, true));
  EXPECT_EQ(FileViewKeyboard::Action::Home, FileViewKeyboard::FromKey(ListBoxKeyboard::kHome, true));
  EXPECT_EQ(FileViewKeyboard::Action::First, FileViewKeyboard::FromKey(ListBoxKeyboard::kHome, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::FromKey(ListBoxKeyboard::kBackSpace, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::FromKey(FileViewKeyboard::kXF86Back, false));
  EXPECT_EQ(FileViewKeyboard::Action::UpDir, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::HistoryBack, false));
  EXPECT_EQ(FileViewKeyboard::Action::HistoryBack, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::HistoryBack, true));
  EXPECT_EQ(FileViewKeyboard::Action::Home, FileViewKeyboard::ResolveHistoryBack(FileViewKeyboard::Action::Home, false));
}

TEST(DeviceKeyboard, MenuTriggerAndTarget) {
  EXPECT_TRUE(DeviceKeyboard::IsMenuTrigger(DeviceKeyboard::kMenuKey, 0));
  EXPECT_TRUE(DeviceKeyboard::IsMenuTrigger(DeviceKeyboard::kF10Key, DeviceKeyboard::kShiftMask));
  EXPECT_FALSE(DeviceKeyboard::IsMenuTrigger(DeviceKeyboard::kF10Key, 0));
  EXPECT_EQ(DeviceKeyboard::MenuTarget::Device, DeviceKeyboard::MenuForSelection(true, true));
  EXPECT_EQ(DeviceKeyboard::MenuTarget::Song, DeviceKeyboard::MenuForSelection(false, true));
  EXPECT_EQ(DeviceKeyboard::MenuTarget::None, DeviceKeyboard::MenuForSelection(false, false));
}

TEST(DeviceKeyboard, DoubleClickOpensDeviceLikeQt) {
  EXPECT_TRUE(DeviceKeyboard::ShouldOpenOnDoubleClick(CollectionTreeClick::kPrimaryButton, 2, true));
  EXPECT_FALSE(DeviceKeyboard::ShouldOpenOnDoubleClick(CollectionTreeClick::kPrimaryButton, 2, false));
  EXPECT_FALSE(DeviceKeyboard::ShouldOpenOnDoubleClick(CollectionTreeClick::kPrimaryButton, 1, true));
  EXPECT_FALSE(DeviceKeyboard::ShouldOpenOnDoubleClick(CollectionTreeClick::kMiddleButton, 2, true));
  EXPECT_TRUE(DeviceKeyboard::ShouldCoalesceDeviceOpen("usb", "usb"));
  EXPECT_FALSE(DeviceKeyboard::ShouldCoalesceDeviceOpen(std::string(), "usb"));
  EXPECT_FALSE(DeviceKeyboard::ShouldCoalesceDeviceOpen("a", "b"));
}

TEST(DeviceKeyboard, FromKeyBackAndSpecialRows) {
  EXPECT_EQ(DeviceKeyboard::Action::Activate, DeviceKeyboard::FromKey(ListBoxKeyboard::kReturn));
  EXPECT_EQ(DeviceKeyboard::Action::MoveUp, DeviceKeyboard::FromKey(ListBoxKeyboard::kUp));
  EXPECT_EQ(DeviceKeyboard::Action::MoveDown, DeviceKeyboard::FromKey(ListBoxKeyboard::kDown));
  EXPECT_EQ(DeviceKeyboard::Action::Home, DeviceKeyboard::FromKey(ListBoxKeyboard::kHome));
  EXPECT_EQ(DeviceKeyboard::Action::End, DeviceKeyboard::FromKey(ListBoxKeyboard::kEnd));
  EXPECT_EQ(DeviceKeyboard::Action::Back, DeviceKeyboard::FromKey(ListBoxKeyboard::kBackSpace));
  EXPECT_EQ(DeviceKeyboard::Action::Escape, DeviceKeyboard::FromKey(ListBoxKeyboard::kEscape));
  EXPECT_EQ(DeviceKeyboard::Action::Expand, DeviceKeyboard::FromKey(ListBoxKeyboard::kRight));
  EXPECT_EQ(DeviceKeyboard::Action::Collapse, DeviceKeyboard::FromKey(ListBoxKeyboard::kLeft));
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
  EXPECT_EQ(4u, DeviceSongMenu::VisibleItems({song}, false).size());
  const CollectionBehaviour::Plan append = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::Append, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_FALSE(append.clear_current);
  EXPECT_EQ(CollectionBehaviour::Destination::Current, append.destination);
  const CollectionBehaviour::Plan replace = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::Replace, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_TRUE(replace.clear_current);
  const CollectionBehaviour::Plan open_new = DeviceSongMenu::PlanFor(DeviceSongMenu::Action::New, BehaviourSettings::PlayBehaviour::Never, true);
  EXPECT_EQ(CollectionBehaviour::Destination::New, open_new.destination);
}

TEST(DeviceCopy, FileViewRequestPassesFilenamesToOrganize) {
  const std::vector<std::string> files = {"/music/album", "/tmp/a.mp3"};
  const OrganizeDialog::Request file_view = DeviceCopy::FileViewRequest(files);
  EXPECT_EQ(files, file_view.filenames);
  EXPECT_TRUE(file_view.songs.empty());
  EXPECT_FALSE(file_view.move);
  EXPECT_TRUE(DeviceCopy::CanCopyFilenames(files));
  EXPECT_FALSE(DeviceCopy::CanCopyFilenames({}));
  EXPECT_TRUE(DeviceCopy::HasOrganizeSource({}, files));
  Song song(Song::Source::LocalFile);
  song.set_valid(true);
  EXPECT_TRUE(DeviceCopy::HasOrganizeSource({song}, {}));
  EXPECT_FALSE(DeviceCopy::HasOrganizeSource({}, {}));
  EXPECT_STREQ("Copy songs", DeviceCopy::CopyButtonLabel(false, true));
  EXPECT_STREQ("Copy songs", DeviceCopy::CopyButtonLabel(true, false));
  EXPECT_STREQ("Copy playlist", DeviceCopy::CopyButtonLabel(false, false));
}

TEST(DeviceCopy, CollectionRequestCopiesWithoutMove) {
  EXPECT_FALSE(DeviceCopy::CanCopyToCollection({}));
  Song song(Song::Source::Device);
  song.set_url("gphoto2://phone/Music/roads.mp3");
  song.set_title("Roads");
  EXPECT_TRUE(DeviceCopy::CanCopyToCollection({song}));
  EXPECT_FALSE(DeviceCopy::CanCopyToCollection({song}, false));
  ConnectedDevice filesystem;
  filesystem.mount_path = "/run/media/usb";
  EXPECT_TRUE(DeviceCopy::IsFilesystemDevice(filesystem));
  ConnectedDevice mtp;
  mtp.backend = "mtp";
  EXPECT_FALSE(DeviceCopy::IsFilesystemDevice(mtp));
  EXPECT_TRUE(DeviceCopy::ShouldUseOrganizeDialog(filesystem));
  EXPECT_FALSE(DeviceCopy::UsesDeviceCopyRunner(filesystem));
  EXPECT_TRUE(DeviceCopy::ShouldUseOrganizeDialog(mtp));
  EXPECT_TRUE(DeviceCopy::UsesDeviceCopyRunner(mtp));
  ConnectedDevice ipod;
  ipod.backend = "gpod";
  ipod.mount_path = "/media/ipod";
  EXPECT_TRUE(DeviceCopy::ShouldUseOrganizeDialog(ipod));
  EXPECT_TRUE(DeviceCopy::UsesDeviceCopyRunner(ipod));
  EXPECT_EQ("serial", DeviceCopyJob::MtpSerial("mtp:serial"));
  EXPECT_EQ("serial", DeviceCopyJob::MtpSerial("MTP/serial"));
  EXPECT_EQ("usb", DeviceCopyJob::MtpSerial("usb"));
  EXPECT_STREQ("Copying to device", DeviceCopyJob::TaskName());
  EXPECT_EQ(10, DeviceCopyJob::kBatchSize);
  EXPECT_TRUE(DeviceCopyJob::ShouldFinish(10, 10, false));
  EXPECT_FALSE(DeviceCopyJob::ShouldFinish(3, 10, false));
  EXPECT_TRUE(DeviceCopyJob::ShouldScheduleNext(0, 12, false, true));
  EXPECT_FALSE(DeviceCopyJob::ShouldScheduleNext(0, 12, false, false));
  EXPECT_FLOAT_EQ(0.5f, DeviceCopyJob::FileFraction(50, 100));
  EXPECT_FLOAT_EQ(0.0f, DeviceCopyJob::FileFraction(1, 0));
  EXPECT_EQ(0.0f, DeviceCopyJob::ClampFileFraction(-1.0f));
  EXPECT_EQ(1.0f, DeviceCopyJob::ClampFileFraction(1.5f));
  EXPECT_EQ(0, DeviceCopyJob::ScaledProgress(0, 0.0f, 0));
  EXPECT_EQ(400, DeviceCopyJob::ScaledProgressMax(4));
  EXPECT_EQ(150, DeviceCopyJob::ScaledProgress(1, 0.5f, 4));
  EXPECT_EQ(400, DeviceCopyJob::ScaledProgress(4, 1.0f, 4));
  const OrganizeDialog::Request request = DeviceCopy::CollectionRequest({song});
  ASSERT_EQ(1u, request.songs.size());
  EXPECT_EQ("Roads", request.songs.front().title());
  EXPECT_FALSE(request.move);
  EXPECT_TRUE(request.destination.empty());
  EXPECT_TRUE(request.playlist.empty());
  EXPECT_TRUE(DeviceCopy::CanCopyToCollection(request.songs));
  EXPECT_EQ("Summer", DeviceCopyPlaylist::NameForCopy("Summer", false, "Other"));
  EXPECT_EQ("Current", DeviceCopyPlaylist::NameForCopy("", true, "Current"));
  EXPECT_TRUE(DeviceCopyPlaylist::NameForCopy("", false, "Current").empty());
  EXPECT_TRUE(DeviceCopyPlaylist::ShouldWriteNamedPlaylist("Summer"));
  EXPECT_FALSE(DeviceCopyPlaylist::ShouldWriteNamedPlaylist(""));

  SongList missing;
  for (int i = 0; i < 12; ++i) {
    Song track;
    track.set_valid(true);
    track.set_title("D" + std::to_string(i));
    track.set_url("file:///tmp/does-not-exist-device-copy-" + std::to_string(i) + ".flac");
    missing.push_back(track);
  }
  TaskManager tasks;
  bool paused = false;
  tasks.PauseCollectionWatchers.Connect([&paused]() { paused = true; });
  DeviceCopyRunner runner(&tasks, nullptr);
  ConnectedDevice none;
  runner.Begin(none, missing);
  runner.ProcessSome();
  EXPECT_EQ(10, runner.next_index());
  EXPECT_FALSE(runner.finished());
  runner.Cancel();
  runner.ProcessSome();
  EXPECT_TRUE(runner.finished());
  EXPECT_EQ(10u, runner.errors().size());

  DeviceCopyRunner sync(&tasks, nullptr);
  EXPECT_FALSE(sync.Copy(none, SongList{missing.front()}));
  EXPECT_TRUE(paused);
  EXPECT_TRUE(sync.finished());
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

TEST(DeviceMenu, UnmountAndForgetFollowQtRules) {
  EXPECT_EQ(5, DeviceMenu::ItemCount());
  EXPECT_EQ(DeviceMenu::Action::Unmount, DeviceMenu::FromId("unmount"));
  EXPECT_EQ(DeviceMenu::Action::Forget, DeviceMenu::FromId("forget"));
  ConnectedDevice plugged;
  plugged.unique_id = "usb:1";
  plugged.mount_path = "/run/media/usb";
  const DeviceMenu::DeviceState connected = DeviceMenu::FromDevice(plugged, false);
  EXPECT_TRUE(connected.connected);
  EXPECT_TRUE(connected.filesystem);
  EXPECT_TRUE(DeviceMenu::UnmountEnabled(connected));
  EXPECT_FALSE(DeviceMenu::ForgetEnabled(connected));
  const auto plugged_items = DeviceMenu::VisibleItems(connected);
  EXPECT_TRUE(DeviceMenu::Contains(plugged_items, DeviceMenu::Action::Browse));
  EXPECT_TRUE(DeviceMenu::Contains(plugged_items, DeviceMenu::Action::Unmount));
  EXPECT_TRUE(DeviceMenu::Contains(plugged_items, DeviceMenu::Action::Properties));
  EXPECT_FALSE(DeviceMenu::Contains(plugged_items, DeviceMenu::Action::Forget));

  ConnectedDevice remembered;
  remembered.unique_id = "usb:1";
  const DeviceMenu::DeviceState stored = DeviceMenu::FromDevice(remembered, true);
  EXPECT_TRUE(DeviceMenu::ForgetEnabled(stored));
  EXPECT_FALSE(DeviceMenu::UnmountEnabled(stored));
  EXPECT_TRUE(DeviceMenu::Contains(DeviceMenu::VisibleItems(stored), DeviceMenu::Action::Forget));
  EXPECT_FALSE(DeviceMenu::Contains(DeviceMenu::VisibleItems(stored), DeviceMenu::Action::Unmount));

  const DeviceMenu::DeviceState offline = DeviceMenu::FromDevice(ConnectedDevice{}, true);
  EXPECT_FALSE(DeviceMenu::UnmountEnabled(offline));
  EXPECT_TRUE(DeviceMenu::ForgetEnabled(offline));
  const auto offline_items = DeviceMenu::VisibleItems(offline);
  EXPECT_FALSE(DeviceMenu::Contains(offline_items, DeviceMenu::Action::Unmount));
  EXPECT_TRUE(DeviceMenu::Contains(offline_items, DeviceMenu::Action::Properties));
}

TEST(FileViewMode, ListAndTreeMatchQt) {
  EXPECT_EQ(FileViewMode::Mode::List, FileViewMode::DefaultMode());
  EXPECT_EQ(FileViewMode::Mode::List, FileViewMode::FromTreeActive(false));
  EXPECT_EQ(FileViewMode::Mode::Tree, FileViewMode::FromTreeActive(true));
  EXPECT_FALSE(FileViewMode::TreeActive(FileViewMode::Mode::List));
  EXPECT_TRUE(FileViewMode::TreeActive(FileViewMode::Mode::Tree));
  EXPECT_EQ(FileViewMode::Mode::Tree, FileViewMode::Toggle(FileViewMode::Mode::List));
  EXPECT_EQ(FileViewMode::Mode::List, FileViewMode::Toggle(FileViewMode::Mode::Tree));
  EXPECT_TRUE(FileViewMode::NavVisible(FileViewMode::Mode::List));
  EXPECT_FALSE(FileViewMode::NavVisible(FileViewMode::Mode::Tree));
  EXPECT_TRUE(FileViewMode::RootButtonsVisible(FileViewMode::Mode::Tree));
  EXPECT_FALSE(FileViewMode::RootButtonsVisible(FileViewMode::Mode::List));
  EXPECT_TRUE(FileViewMode::ActivateNavigates(FileViewMode::Mode::List, true));
  EXPECT_FALSE(FileViewMode::ActivateNavigates(FileViewMode::Mode::Tree, true));
  EXPECT_FALSE(FileViewMode::ActivateAddsToPlaylist(FileViewMode::Mode::List, false));
  EXPECT_FALSE(FileViewMode::ActivateAddsToPlaylist(FileViewMode::Mode::Tree, false));
  EXPECT_FALSE(FileViewMode::ActivateAddsToPlaylist(FileViewMode::Mode::Tree, true));
  EXPECT_TRUE(FileViewMode::DoubleClickAddsToPlaylist(false));
  EXPECT_FALSE(FileViewMode::DoubleClickAddsToPlaylist(true));
}

TEST(FileViewIcons, UsesSymbolicIconsLikeQFileIconProvider) {
  EXPECT_STREQ("folder-symbolic", FileViewIcons::IconName(true, "/music"));
  EXPECT_STREQ("audio-x-generic-symbolic", FileViewIcons::IconName(false, "/music/roads.flac"));
  EXPECT_STREQ("view-list-symbolic", FileViewIcons::IconName(false, "/music/album.m3u"));
  EXPECT_STREQ("text-x-generic-symbolic", FileViewIcons::IconName(false, "/music/notes.txt"));
}

TEST(FileViewMode, RootsAddRemoveAndEncode) {
  EXPECT_EQ("/music", FileViewMode::CleanPath("/music/"));
  EXPECT_EQ("/", FileViewMode::CleanPath("/"));
  EXPECT_TRUE(FileViewMode::SamePath("/music/", "/music"));
  EXPECT_TRUE(FileViewMode::PathUnderRoot("/music/portishead", "/music"));
  EXPECT_TRUE(FileViewMode::PathUnderRoot("/music", "/music"));
  EXPECT_FALSE(FileViewMode::PathUnderRoot("/music2", "/music"));
  const std::vector<std::string> added = FileViewMode::AddRoot({"/music"}, "/home/user");
  ASSERT_EQ(2u, added.size());
  EXPECT_EQ("/home/user", added.back());
  EXPECT_EQ(1u, FileViewMode::AddRoot({"/music"}, "/music/").size());
  EXPECT_EQ(1u, FileViewMode::AddRoot({"/music"}, {}).size());
  EXPECT_EQ("/music", FileViewMode::MatchingRoot({"/music", "/home"}, "/music/portishead/dummy"));
  EXPECT_TRUE(FileViewMode::MatchingRoot({"/music"}, "/other").empty());
  const std::vector<std::string> removed = FileViewMode::RemoveMatchingRoot({"/music", "/home"}, "/music/portishead");
  ASSERT_EQ(1u, removed.size());
  EXPECT_EQ("/home", removed.front());
  EXPECT_EQ("/music\n/home", FileViewMode::EncodeRoots({"/music", "/home"}));
  const std::vector<std::string> decoded = FileViewMode::DecodeRoots("/music\n\n /home \n");
  ASSERT_EQ(2u, decoded.size());
  EXPECT_EQ("/music", decoded[0]);
  EXPECT_EQ("/home", decoded[1]);
  EXPECT_TRUE(FileViewMode::DecodeRoots("").empty());
  ASSERT_EQ(1u, FileViewMode::DefaultRoots("/music").size());
  EXPECT_EQ("/music", FileViewMode::DefaultRoots("/music").front());
  EXPECT_TRUE(FileViewMode::DefaultRoots({}).empty());
}

TEST(FileViewMode, RightClickKeepsMultiSelection) {
  EXPECT_TRUE(FileViewMode::ReplaceSelection(false));
  EXPECT_FALSE(FileViewMode::ReplaceSelection(true));
  const std::vector<std::string> selected = {"/a.flac", "/b.flac"};
  EXPECT_EQ(selected, FileViewMode::MenuPaths(selected, "/a.flac"));
  ASSERT_EQ(1u, FileViewMode::MenuPaths(selected, "/c.flac").size());
  EXPECT_EQ("/c.flac", FileViewMode::MenuPaths(selected, "/c.flac").front());
  EXPECT_EQ(selected, FileViewMode::MenuPaths(selected, {}));
}

TEST(FileViewTreeModel, LazyLoadIncludesAudioFiles) {
  const std::string dir = TempDir();
  const std::string audio = FileUtils::Join(dir, "roads.flac");
  const std::string notes = FileUtils::Join(dir, "notes.txt");
  const std::string nested = FileUtils::Join(dir, "dummy");
  ASSERT_TRUE(FileUtils::WriteFile(audio, "a"));
  ASSERT_TRUE(FileUtils::WriteFile(notes, "b"));
  ASSERT_EQ(0, mkdir(nested.c_str(), 0755));

  FileViewTreeModel model;
  model.SetRootPaths({dir});
  ASSERT_EQ(1, model.DirectoryCount());
  FileViewTreeItem *root_dir = model.root()->children.front().get();
  model.LazyLoad(root_dir);
  bool saw_audio = false;
  bool saw_notes = false;
  bool saw_nested = false;
  for (const auto &child : root_dir->children) {
    if (child->path == audio) {
      saw_audio = true;
      EXPECT_EQ(FileViewTreeItem::Type::File, child->type);
    }
    if (child->path == notes) {
      saw_notes = true;
    }
    if (child->path == nested) {
      saw_nested = true;
      EXPECT_EQ(FileViewTreeItem::Type::Directory, child->type);
    }
  }
  EXPECT_TRUE(saw_audio);
  EXPECT_FALSE(saw_notes);
  EXPECT_TRUE(saw_nested);

  FileUtils::Remove(audio);
  FileUtils::Remove(notes);
  rmdir(nested.c_str());
  rmdir(dir.c_str());
}

TEST(DeviceViewLook, IconsAndStatusMatchQtDelegate) {
  EXPECT_EQ(32, DeviceViewLook::kIconSize);
  ConnectedDevice mtp;
  mtp.backend = "mtp";
  mtp.icon = "multimedia-player-symbolic";
  EXPECT_STREQ("multimedia-player-symbolic", DeviceViewLook::IconName(mtp));
  EXPECT_EQ(DeviceViewLook::Status::NotConnected, DeviceViewLook::InferStatus(mtp));
  EXPECT_EQ("Double click to open", DeviceViewLook::StatusText(mtp));

  ConnectedDevice volume;
  volume.backend = "udisks2";
  volume.friendly_name = "USB Drive";
  EXPECT_STREQ("drive-harddisk-usb-symbolic", DeviceViewLook::IconName(volume));
  EXPECT_EQ(DeviceViewLook::Status::NotMounted, DeviceViewLook::InferStatus(volume));
  EXPECT_EQ("Not mounted - double click to mount", DeviceViewLook::StatusText(volume));

  volume.mount_path = "/run/media/usb";
  EXPECT_EQ(DeviceViewLook::Status::Connected, DeviceViewLook::InferStatus(volume));
  EXPECT_EQ("/run/media/usb", DeviceViewLook::StatusText(volume));
  EXPECT_EQ("1 song", DeviceViewLook::StatusText(volume, 1));
  EXPECT_EQ("12 songs", DeviceViewLook::StatusText(volume, 12));
  EXPECT_EQ("Not connected", DeviceViewLook::StatusText(volume, -1, true));
  EXPECT_EQ("Updating 40%...", DeviceViewLook::UpdatingText(40));
  EXPECT_EQ("Updating 0%...", DeviceViewLook::UpdatingText(-8));
  EXPECT_EQ("Updating 100%...", DeviceViewLook::UpdatingText(140));
  volume.song_count = 12;
  volume.updating_percent = 40;
  EXPECT_EQ("Updating 40%...", DeviceViewLook::RowStatusText(volume));
  volume.updating_percent = -1;
  EXPECT_EQ("12 songs", DeviceViewLook::RowStatusText(volume));
  volume.mount_path.clear();
  EXPECT_TRUE(DeviceViewLook::ShouldMountOnActivate(volume));
  EXPECT_TRUE(DeviceViewLook::ShouldConnectOnDoubleClick(DeviceViewLook::Status::NotMounted));
  EXPECT_TRUE(DeviceViewLook::ShouldConnectOnDoubleClick(DeviceViewLook::Status::NotConnected));
  EXPECT_TRUE(DeviceViewLook::ShouldConnectOnDoubleClick(DeviceViewLook::Status::Remembered));
  EXPECT_FALSE(DeviceViewLook::ShouldConnectOnDoubleClick(DeviceViewLook::Status::Connected));
  volume.mount_path = "/run/media/usb";
  EXPECT_FALSE(DeviceViewLook::ShouldMountOnActivate(volume));
  volume.mount_path.clear();
  volume.remembered = true;
  EXPECT_FALSE(DeviceViewLook::ShouldMountOnActivate(volume));

  ConnectedDevice cd;
  cd.backend = "cdda";
  EXPECT_STREQ("media-optical-symbolic", DeviceViewLook::IconName(cd));

  CollectionItem folder(CollectionItem::Type::Container);
  EXPECT_STREQ("folder-symbolic", DeviceViewLook::ItemIconName(&folder));
  CollectionItem song(CollectionItem::Type::Song);
  song.metadata.set_url("file:///music/roads.flac");
  EXPECT_STREQ("audio-x-generic-symbolic", DeviceViewLook::ItemIconName(&song));
}

TEST(DeviceScanProgress, MapsTaskToPercent) {
  EXPECT_STREQ("Scanning device", DeviceScanProgress::TaskName());
  EXPECT_TRUE(DeviceScanProgress::IsScanTask("Scanning device"));
  EXPECT_EQ(0, DeviceScanProgress::Percent(0, 0));
  EXPECT_EQ(40, DeviceScanProgress::Percent(40, 100));
  EXPECT_EQ(100, DeviceScanProgress::Percent(12, 10));
  EXPECT_TRUE(DeviceScanProgress::ShouldReport(-1, 0));
  EXPECT_FALSE(DeviceScanProgress::ShouldReport(40, 40));
  EXPECT_EQ(-1, DeviceScanProgress::FinishedPercent());
}

TEST(DeviceViewReload, DoesNotRescanOnRefresh) {
  EXPECT_FALSE(DeviceViewReload::ShouldRescanOnReload());
}

TEST(DeviceCopyRefresh, ShouldRefreshAfterCopyMatchesQtFinishCopy) {
  EXPECT_FALSE(DeviceCopyRefresh::ShouldRefreshAfterCopy("mtp", 0));
  EXPECT_TRUE(DeviceCopyRefresh::ShouldRefreshAfterCopy("mtp", 1));
  EXPECT_TRUE(DeviceCopyRefresh::ShouldRefreshAfterCopy("gpod", 3));
  EXPECT_TRUE(DeviceCopyRefresh::ShouldRefreshAfterCopy("gio", 1));
  EXPECT_TRUE(DeviceCopyRefresh::ShouldRefreshAfterCopy("udisks2", 2));
  EXPECT_FALSE(DeviceCopyRefresh::ShouldRefreshAfterCopy("cdda", 4));
  EXPECT_FALSE(DeviceCopyRefresh::ShouldRefreshAfterCopy("", 2));
  Song mtp(Song::Source::Device);
  mtp.set_artist("Artist");
  mtp.set_albumartist("Album Artist");
  DeviceCopyRefresh::ApplyMtpCollectionFields(&mtp);
  EXPECT_EQ(1, mtp.directory_id());
  EXPECT_EQ("Album Artist", mtp.artist());
  EXPECT_EQ("", mtp.albumartist());
  Song ipod(Song::Source::Device);
  DeviceCopyRefresh::ApplyGPodCollectionFields(&ipod);
  EXPECT_EQ(1, ipod.directory_id());
}

TEST(DeviceForgetDialog, MatchesQtForgetCopy) {
  EXPECT_STREQ("Forget device", DeviceForgetDialog::Title());
  EXPECT_STREQ("Forget device", DeviceForgetDialog::Accept());
  EXPECT_STREQ("Cancel", DeviceForgetDialog::Cancel());
  EXPECT_NE(std::string::npos, std::string(DeviceForgetDialog::Message()).find("rescan all the songs again"));
  EXPECT_FALSE(DeviceForgetDialog::NeedsPrompt("cdda"));
  EXPECT_TRUE(DeviceForgetDialog::NeedsPrompt("mtp"));
}

TEST(DeviceDeleteDialog, MatchesQtDeleteCopy) {
  EXPECT_STREQ("Delete files", DeviceDeleteDialog::Title());
  EXPECT_STREQ("These files will be deleted from the device, are you sure you want to continue?", DeviceDeleteDialog::Message());
  EXPECT_STREQ("Yes", DeviceDeleteDialog::Accept());
  EXPECT_STREQ("Cancel", DeviceDeleteDialog::Cancel());
}

TEST(DeviceConnectDialog, MatchesQtFirstConnectCopy) {
  EXPECT_STREQ("Connect device", DeviceConnectDialog::Title());
  EXPECT_STREQ("Connect device", DeviceConnectDialog::Accept());
  EXPECT_STREQ("Cancel", DeviceConnectDialog::Cancel());
  EXPECT_NE(std::string::npos, std::string(DeviceConnectDialog::Message()).find("first time you have connected this device"));
  EXPECT_FALSE(DeviceConnectDialog::AskForScan("cdda"));
  EXPECT_TRUE(DeviceConnectDialog::AskForScan("mtp"));
  EXPECT_TRUE(DeviceConnectDialog::NeedsFirstConnectPrompt(false, false, "mtp"));
  EXPECT_FALSE(DeviceConnectDialog::NeedsFirstConnectPrompt(true, false, "mtp"));
  EXPECT_FALSE(DeviceConnectDialog::NeedsFirstConnectPrompt(false, true, "gio"));
  EXPECT_FALSE(DeviceConnectDialog::NeedsFirstConnectPrompt(false, false, "cdda"));
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

TEST(DevicePropertiesIcons, MatchQtIconList) {
  ASSERT_EQ(7u, DevicePropertiesIcons::Names().size());
  EXPECT_STREQ("device", DevicePropertiesIcons::Names().front());
  EXPECT_STREQ("device-phone", DevicePropertiesIcons::Names().back());
  EXPECT_EQ(0, DevicePropertiesIcons::IndexOf("device"));
  EXPECT_EQ(6, DevicePropertiesIcons::IndexOf("device-phone"));
  EXPECT_EQ(-1, DevicePropertiesIcons::IndexOf("unknown-icon"));
  EXPECT_STREQ("device-usb-flash", DevicePropertiesIcons::IconAt(2));
  EXPECT_EQ("device", DevicePropertiesIcons::EffectiveIcon("missing"));
  EXPECT_EQ("device-ipod", DevicePropertiesIcons::EffectiveIcon("device-ipod"));
  EXPECT_STREQ("drive-harddisk-symbolic", DevicePropertiesIcons::GtkName("device"));
  EXPECT_STREQ("phone-symbolic", DevicePropertiesIcons::GtkName("device-phone"));
}

TEST(DevicePropertiesLabels, TabsModesAndSortedFormats) {
  EXPECT_STREQ("Information", DevicePropertiesLabels::InformationTab());
  EXPECT_STREQ("File formats", DevicePropertiesLabels::FileFormatsTab());
  EXPECT_STREQ("Do not convert any music", DevicePropertiesLabels::Never());
  EXPECT_STREQ("Convert any music that the device can't play", DevicePropertiesLabels::Unsupported());
  EXPECT_STREQ("Convert all music", DevicePropertiesLabels::Always());
  EXPECT_STREQ("Preferred format", DevicePropertiesLabels::PreferredFormat());
  EXPECT_STREQ("Supported formats", DevicePropertiesLabels::SupportedFormats());
  EXPECT_STREQ("This device supports the following file formats:", DevicePropertiesLabels::SupportedFormatsIntro());
  EXPECT_STREQ("Querying device...", DevicePropertiesLabels::QueryingDevice());
  EXPECT_STREQ("Open device", DevicePropertiesLabels::OpenDevice());
  EXPECT_FALSE(DevicePropertiesLabels::UnsupportedEnabled(false));
  EXPECT_TRUE(DevicePropertiesLabels::UnsupportedEnabled(true));
  EXPECT_FALSE(DevicePropertiesLabels::SupportedListVisible(false));
  EXPECT_TRUE(DevicePropertiesLabels::SupportedListVisible(true));
  EXPECT_TRUE(DevicePropertiesLabels::ShouldFallbackToNever(true, false));
  EXPECT_FALSE(DevicePropertiesLabels::ShouldFallbackToNever(true, true));
  EXPECT_FALSE(DevicePropertiesLabels::ShouldFallbackToNever(false, false));
  EXPECT_TRUE(DevicePropertiesLabels::ShouldPickBestFormat(false, Song::FileType::FLAC));
  EXPECT_FALSE(DevicePropertiesLabels::ShouldPickBestFormat(true, Song::FileType::FLAC));
  EXPECT_TRUE(DevicePropertiesLabels::ShouldPickBestFormat(true, Song::FileType::Unknown));
  const auto names = DevicePropertiesLabels::SupportedFormatNames({Song::FileType::FLAC, Song::FileType::MPEG});
  ASSERT_EQ(2u, names.size());
  EXPECT_EQ("FLAC", names.front());
  EXPECT_EQ("MP3", names.back());
  EXPECT_EQ(0, DevicePropertiesLabels::RadioIndex(DeviceDatabaseBackend::TranscodeMode::Transcode_Never));
  EXPECT_EQ(1, DevicePropertiesLabels::RadioIndex(DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported));
  EXPECT_EQ(2, DevicePropertiesLabels::RadioIndex(DeviceDatabaseBackend::TranscodeMode::Transcode_Always));
  EXPECT_EQ(DeviceDatabaseBackend::TranscodeMode::Transcode_Never, DevicePropertiesLabels::ModeFromRadio(0));
  EXPECT_EQ(DeviceDatabaseBackend::TranscodeMode::Transcode_Unsupported, DevicePropertiesLabels::ModeFromRadio(1));
  EXPECT_EQ(DeviceDatabaseBackend::TranscodeMode::Transcode_Always, DevicePropertiesLabels::ModeFromRadio(2));
  EXPECT_EQ(Song::FileType::MPEG, DevicePropertiesLabels::FileTypeFor(Transcoder::Format::MP3));
  EXPECT_EQ(Song::FileType::MP4, DevicePropertiesLabels::FileTypeFor(Transcoder::Format::AAC));
  EXPECT_EQ(Song::FileType::OggOpus, DevicePropertiesLabels::FileTypeFor(Transcoder::Format::Opus));
  const auto formats = DevicePropertiesLabels::FormatChoices();
  ASSERT_EQ(11u, formats.size());
  EXPECT_EQ("AAC", formats.front().second);
  EXPECT_EQ("WavPack", formats.back().second);
  EXPECT_EQ(Song::FileType::MP4, DevicePropertiesLabels::FormatAt(0));
  EXPECT_GE(DevicePropertiesLabels::IndexOfFormat(Song::FileType::FLAC), 0);
}

TEST(DevicePropertiesInfo, HardwareRowsAndOpenRule) {
  ConnectedDevice empty;
  EXPECT_FALSE(DevicePropertiesInfo::HasHardwareInfo(empty));
  EXPECT_FALSE(DevicePropertiesInfo::OpenEnabled(empty));
  EXPECT_TRUE(DevicePropertiesInfo::Rows(empty).empty());
  EXPECT_FALSE(DevicePropertiesInfo::SpaceFor(empty).available);

  ConnectedDevice device;
  device.unique_id = "usb:1234";
  device.backend = "gio";
  device.mount_path = "/media/music";
  device.size = 1024;
  device.icon = "device-usb-drive";
  EXPECT_TRUE(DevicePropertiesInfo::HasHardwareInfo(device));
  EXPECT_TRUE(DevicePropertiesInfo::OpenEnabled(device));
  const auto rows = DevicePropertiesInfo::Rows(device);
  ASSERT_EQ(5u, rows.size());
  EXPECT_EQ("Backend", rows.front().key);
  EXPECT_EQ("Unique ID", rows.back().key);
  EXPECT_EQ("usb:1234", rows.back().value);
}

TEST(DeviceSupportedFormats, PagesOpenAndResolveMatchQt) {
  EXPECT_STREQ("Querying device...", DeviceSupportedFormats::QueryingDevice());
  EXPECT_STREQ("This device supports the following file formats:", DeviceSupportedFormats::SupportedFormatsIntro());
  EXPECT_STREQ("not-connected", DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::NotConnected));
  EXPECT_STREQ("loading", DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::Loading));
  EXPECT_STREQ("formats", DeviceSupportedFormats::StackName(DeviceSupportedFormats::Page::Formats));

  ConnectedDevice empty;
  EXPECT_FALSE(DeviceSupportedFormats::PhysicallyPresent(empty));
  EXPECT_FALSE(DeviceSupportedFormats::Opened(empty));
  EXPECT_FALSE(DeviceSupportedFormats::OpenEnabled(empty));
  EXPECT_EQ(DeviceSupportedFormats::Page::NotConnected, DeviceSupportedFormats::PageFor(empty));

  ConnectedDevice remembered;
  remembered.unique_id = "usb:old";
  remembered.backend = "gio";
  remembered.remembered = true;
  EXPECT_FALSE(DeviceSupportedFormats::PhysicallyPresent(remembered));
  EXPECT_FALSE(DeviceSupportedFormats::OpenEnabled(remembered));
  EXPECT_EQ(DeviceSupportedFormats::Page::NotConnected, DeviceSupportedFormats::PageFor(remembered));

  ConnectedDevice unmounted;
  unmounted.unique_id = "usb:1234";
  unmounted.backend = "gio";
  EXPECT_TRUE(DeviceSupportedFormats::PhysicallyPresent(unmounted));
  EXPECT_FALSE(DeviceSupportedFormats::Opened(unmounted));
  EXPECT_TRUE(DeviceSupportedFormats::OpenEnabled(unmounted));
  EXPECT_EQ(DeviceSupportedFormats::Page::NotConnected, DeviceSupportedFormats::PageFor(unmounted));

  ConnectedDevice mounted = unmounted;
  mounted.mount_path = "/media/music";
  EXPECT_TRUE(DeviceSupportedFormats::Opened(mounted));
  EXPECT_FALSE(DeviceSupportedFormats::OpenEnabled(mounted));
  EXPECT_FALSE(DeviceSupportedFormats::ShouldQuery(mounted));
  EXPECT_EQ(DeviceSupportedFormats::Page::Formats, DeviceSupportedFormats::PageFor(mounted));

  ConnectedDevice mtp;
  mtp.unique_id = "mtp:serial";
  mtp.backend = "mtp";
  EXPECT_TRUE(DeviceSupportedFormats::Opened(mtp));
  EXPECT_FALSE(DeviceSupportedFormats::OpenEnabled(mtp));
  EXPECT_TRUE(DeviceSupportedFormats::ShouldQuery(mtp));
  EXPECT_EQ(DeviceSupportedFormats::Page::Loading, DeviceSupportedFormats::PageFor(mtp));
  EXPECT_EQ(DeviceSupportedFormats::Page::Formats, DeviceSupportedFormats::PageFor(mtp, true));

  const auto gpod = DeviceSupportedFormats::GPodFormats();
  ASSERT_EQ(3u, gpod.size());
  EXPECT_EQ(Song::FileType::MP4, gpod[0]);
  EXPECT_EQ(Song::FileType::MPEG, gpod[1]);
  EXPECT_EQ(Song::FileType::ALAC, gpod[2]);
  EXPECT_EQ(gpod, DeviceSupportedFormats::Resolve("gpod", {}, false, false));
  EXPECT_TRUE(DeviceSupportedFormats::Resolve("gio", {}, false, false).empty());
  EXPECT_TRUE(DeviceSupportedFormats::Resolve("mtp", {}, false, false).empty());
  EXPECT_TRUE(DeviceSupportedFormats::Resolve("mtp", {Song::FileType::FLAC}, false, true).empty());
  const auto queried = DeviceSupportedFormats::Resolve("mtp", {Song::FileType::MPEG, Song::FileType::MPEG, Song::FileType::FLAC}, true, true);
  ASSERT_EQ(2u, queried.size());
  EXPECT_EQ(Song::FileType::MPEG, queried[0]);
  EXPECT_EQ(Song::FileType::FLAC, queried[1]);
}

TEST(DeviceCopySupported, ForCopyPrefersQueriedElseBackend) {
  const auto queried = DeviceCopySupported::ForCopy("mtp", {Song::FileType::MPEG});
  ASSERT_EQ(1u, queried.size());
  EXPECT_EQ(Song::FileType::MPEG, queried[0]);
  const auto fallback = DeviceCopySupported::ForCopy("mtp", {});
  EXPECT_TRUE(OrganizeTranscode::Contains(fallback, Song::FileType::FLAC));
  EXPECT_TRUE(OrganizeTranscode::Contains(fallback, Song::FileType::ASF));
  EXPECT_TRUE(DeviceCopySupported::ForCopy("gio", {}).empty());

  // A queried MPEG-only device transcodes FLAC; the hardcoded MTP fallback does not.
  EXPECT_EQ(Song::FileType::MPEG, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Unsupported,
                                                          Song::FileType::MPEG, DeviceCopySupported::ForCopy("mtp", {Song::FileType::MPEG})));
  EXPECT_EQ(Song::FileType::Unknown, OrganizeTranscode::Check(Song::FileType::FLAC, MusicStorage::TranscodeMode::Transcode_Unsupported,
                                                             Song::FileType::MPEG, DeviceCopySupported::ForCopy("mtp", {})));
}
