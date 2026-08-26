#include "core/song.h"
#include "playlist/playlistmenu.h"

#include <gtest/gtest.h>

namespace {

Song LocalSong() {
  Song song(Song::Source::LocalFile);
  song.set_valid(true);
  song.set_url("file:///tmp/roads.flac");
  song.set_title("Roads");
  return song;
}

Song CollectionSong() {
  Song song(Song::Source::Collection);
  song.set_valid(true);
  song.set_id(7);
  song.set_url("file:///music/roads.flac");
  song.set_title("Roads");
  return song;
}

Song StreamSong() {
  Song song(Song::Source::Tidal);
  song.set_valid(true);
  song.set_url("tidal://track/1");
  song.set_title("Roads");
  return song;
}

Song CueSong() {
  Song song(Song::Source::LocalFile);
  song.set_valid(true);
  song.set_url("file:///tmp/album.flac");
  song.set_cue_path("/tmp/album.cue");
  return song;
}

PlaylistMenu::SelectionState OptsFor(const std::vector<PlaylistMenu::RowInfo> &rows, bool collection_item = false,
                                     bool track_column = false, PlaylistColumn column = PlaylistColumn::Title) {
  PlaylistMenu::SelectionState opts;
  opts.index_valid = !rows.empty();
  opts.collection_item = collection_item;
  opts.track_column = track_column;
  opts.column_editable = PlaylistDelegates::ColumnIsEditable(column);
  opts.column = column;
  opts.column_name = PlaylistDelegates::ColumnTitle(column);
  opts.column_value = "Roads";
  opts.delete_allowed = true;
  opts.devices_connected = true;
  return PlaylistMenu::Analyze(rows, opts);
}

}  // namespace

TEST(PlaylistMenu, CatalogMatchesQt) {
  EXPECT_EQ(28, PlaylistMenu::ItemCount());
  EXPECT_EQ(PlaylistMenu::Action::Play, PlaylistMenu::FromId("play"));
  EXPECT_EQ(PlaylistMenu::Action::Organize, PlaylistMenu::FromId("organize"));
  EXPECT_EQ(PlaylistMenu::Action::CopyToDevice, PlaylistMenu::FromId("copy-device"));
  EXPECT_EQ(PlaylistMenu::Action::ClearPlaylist, PlaylistMenu::FromId("clear"));
  EXPECT_STREQ("win.stop", PlaylistMenu::WinAction(PlaylistMenu::Action::Stop));
  EXPECT_STREQ("win.organize-selected", PlaylistMenu::WinAction(PlaylistMenu::Action::Organize));
  EXPECT_STREQ("win.playlist-copy-device", PlaylistMenu::WinAction(PlaylistMenu::Action::CopyToDevice));
  EXPECT_STREQ("", PlaylistMenu::WinAction(PlaylistMenu::Action::AddToPlaylist));
}

TEST(PlaylistMenu, EmptySelectionKeepsMaintenanceItems) {
  const auto items = PlaylistMenu::VisibleItems(PlaylistMenu::Analyze({}));
  EXPECT_EQ(7u, items.size());
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::Play));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::Stop));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::StopAfter));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::ClearPlaylist));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::Shuffle));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::RemoveDuplicates));
  EXPECT_TRUE(PlaylistMenu::Contains(items, PlaylistMenu::Action::RemoveUnavailable));
  EXPECT_FALSE(PlaylistMenu::Contains(items, PlaylistMenu::Action::Queue));
  EXPECT_FALSE(PlaylistMenu::Contains(items, PlaylistMenu::Action::Remove));
  EXPECT_FALSE(PlaylistMenu::PlayEnabled(PlaylistMenu::Analyze({})));
}

TEST(PlaylistMenu, QueueLabelsMatchQt) {
  PlaylistMenu::RowInfo queued = PlaylistMenu::FromSong(LocalSong(), true);
  PlaylistMenu::RowInfo unqueued = PlaylistMenu::FromSong(LocalSong(), false);
  EXPECT_EQ("Dequeue track", PlaylistMenu::LabelFor(PlaylistMenu::Action::Queue, OptsFor({queued})));
  EXPECT_EQ("Dequeue selected tracks", PlaylistMenu::LabelFor(PlaylistMenu::Action::Queue, OptsFor({queued, queued})));
  EXPECT_EQ("Queue track", PlaylistMenu::LabelFor(PlaylistMenu::Action::Queue, OptsFor({unqueued})));
  EXPECT_EQ("Queue selected tracks", PlaylistMenu::LabelFor(PlaylistMenu::Action::Queue, OptsFor({unqueued, unqueued})));
  EXPECT_EQ("Toggle queue status", PlaylistMenu::LabelFor(PlaylistMenu::Action::Queue, OptsFor({queued, unqueued})));
  EXPECT_EQ("Queue to play next", PlaylistMenu::LabelFor(PlaylistMenu::Action::QueueNext, OptsFor({unqueued})));
  EXPECT_EQ("Queue selected tracks to play next", PlaylistMenu::LabelFor(PlaylistMenu::Action::QueueNext, OptsFor({unqueued, unqueued})));
}

TEST(PlaylistMenu, SkipLabelsMatchQt) {
  PlaylistMenu::RowInfo skipped = PlaylistMenu::FromSong(LocalSong());
  skipped.skipped = true;
  PlaylistMenu::RowInfo playing = PlaylistMenu::FromSong(LocalSong());
  EXPECT_EQ("Unskip track", PlaylistMenu::LabelFor(PlaylistMenu::Action::Skip, OptsFor({skipped})));
  EXPECT_EQ("Unskip selected tracks", PlaylistMenu::LabelFor(PlaylistMenu::Action::Skip, OptsFor({skipped, skipped})));
  EXPECT_EQ("Skip track", PlaylistMenu::LabelFor(PlaylistMenu::Action::Skip, OptsFor({playing})));
  EXPECT_EQ("Skip selected tracks", PlaylistMenu::LabelFor(PlaylistMenu::Action::Skip, OptsFor({playing, playing})));
  EXPECT_EQ("Toggle skip status", PlaylistMenu::LabelFor(PlaylistMenu::Action::Skip, OptsFor({skipped, playing})));
}

TEST(PlaylistMenu, LocalEditableGates) {
  const auto local = PlaylistMenu::VisibleItems(OptsFor({PlaylistMenu::FromSong(LocalSong())}));
  EXPECT_TRUE(PlaylistMenu::Contains(local, PlaylistMenu::Action::EditTags));
  EXPECT_TRUE(PlaylistMenu::Contains(local, PlaylistMenu::Action::Rescan));
  EXPECT_TRUE(PlaylistMenu::Contains(local, PlaylistMenu::Action::Transcode));
  EXPECT_TRUE(PlaylistMenu::Contains(local, PlaylistMenu::Action::AutoCompleteTags));
  EXPECT_FALSE(PlaylistMenu::Contains(local, PlaylistMenu::Action::FetchMetadata));
  EXPECT_FALSE(PlaylistMenu::Contains(local, PlaylistMenu::Action::RenumberTracks));

  const auto stream = PlaylistMenu::VisibleItems(OptsFor({PlaylistMenu::FromSong(StreamSong())}));
  EXPECT_FALSE(PlaylistMenu::Contains(stream, PlaylistMenu::Action::EditTags));
  EXPECT_FALSE(PlaylistMenu::Contains(stream, PlaylistMenu::Action::Rescan));
  EXPECT_TRUE(PlaylistMenu::Contains(stream, PlaylistMenu::Action::FetchMetadata));
  EXPECT_FALSE(PlaylistMenu::Contains(stream, PlaylistMenu::Action::OpenInFileManager));
}

TEST(PlaylistMenu, CollectionVsExternal) {
  const auto collection = PlaylistMenu::VisibleItems(OptsFor({PlaylistMenu::FromSong(CollectionSong())}, true));
  EXPECT_TRUE(PlaylistMenu::Contains(collection, PlaylistMenu::Action::Organize));
  EXPECT_TRUE(PlaylistMenu::Contains(collection, PlaylistMenu::Action::ShowInCollection));
  EXPECT_FALSE(PlaylistMenu::Contains(collection, PlaylistMenu::Action::CopyToCollection));
  EXPECT_FALSE(PlaylistMenu::Contains(collection, PlaylistMenu::Action::MoveToCollection));
  EXPECT_TRUE(PlaylistMenu::Contains(collection, PlaylistMenu::Action::CopyToDevice));

  const auto external = PlaylistMenu::VisibleItems(OptsFor({PlaylistMenu::FromSong(LocalSong())}, false));
  EXPECT_FALSE(PlaylistMenu::Contains(external, PlaylistMenu::Action::Organize));
  EXPECT_FALSE(PlaylistMenu::Contains(external, PlaylistMenu::Action::ShowInCollection));
  EXPECT_TRUE(PlaylistMenu::Contains(external, PlaylistMenu::Action::CopyToCollection));
  EXPECT_TRUE(PlaylistMenu::Contains(external, PlaylistMenu::Action::MoveToCollection));
}

TEST(PlaylistMenu, RenumberAndSetColumn) {
  const auto two_local = std::vector<PlaylistMenu::RowInfo>{PlaylistMenu::FromSong(LocalSong()), PlaylistMenu::FromSong(LocalSong())};
  const auto track = PlaylistMenu::VisibleItems(OptsFor(two_local, false, true, PlaylistColumn::Track));
  EXPECT_TRUE(PlaylistMenu::Contains(track, PlaylistMenu::Action::RenumberTracks));
  EXPECT_FALSE(PlaylistMenu::Contains(track, PlaylistMenu::Action::SetColumnValue));

  PlaylistMenu::SelectionState title = OptsFor(two_local, false, false, PlaylistColumn::Title);
  title.column_value = "A very long title that should be truncated for the menu";
  const auto title_items = PlaylistMenu::VisibleItems(title);
  EXPECT_FALSE(PlaylistMenu::Contains(title_items, PlaylistMenu::Action::RenumberTracks));
  EXPECT_TRUE(PlaylistMenu::Contains(title_items, PlaylistMenu::Action::SetColumnValue));
  EXPECT_EQ("Set title to \"A very long title that sh...\"...", PlaylistMenu::LabelFor(PlaylistMenu::Action::SetColumnValue, title));
  EXPECT_EQ("Edit tag \"Title\"...", PlaylistMenu::LabelFor(PlaylistMenu::Action::EditValue, title));
}

TEST(PlaylistMenu, CopyToDeviceAndDeletePolicy) {
  PlaylistMenu::SelectionState allowed = OptsFor({PlaylistMenu::FromSong(LocalSong())});
  EXPECT_TRUE(PlaylistMenu::Contains(PlaylistMenu::VisibleItems(allowed), PlaylistMenu::Action::CopyToDevice));
  EXPECT_TRUE(PlaylistMenu::Contains(PlaylistMenu::VisibleItems(allowed), PlaylistMenu::Action::DeleteFiles));
  EXPECT_TRUE(PlaylistMenu::CopyToDeviceEnabled(allowed));

  allowed.devices_connected = false;
  EXPECT_TRUE(PlaylistMenu::Contains(PlaylistMenu::VisibleItems(allowed), PlaylistMenu::Action::CopyToDevice));
  EXPECT_FALSE(PlaylistMenu::CopyToDeviceEnabled(allowed));

  allowed.delete_allowed = false;
  EXPECT_FALSE(PlaylistMenu::Contains(PlaylistMenu::VisibleItems(allowed), PlaylistMenu::Action::DeleteFiles));
}

TEST(PlaylistMenu, CueHidesEditsAndPlayPauseLabel) {
  const auto cue = PlaylistMenu::VisibleItems(OptsFor({PlaylistMenu::FromSong(CueSong())}));
  EXPECT_FALSE(PlaylistMenu::Contains(cue, PlaylistMenu::Action::EditTags));
  EXPECT_FALSE(PlaylistMenu::Contains(cue, PlaylistMenu::Action::EditValue));
  EXPECT_FALSE(PlaylistMenu::Contains(cue, PlaylistMenu::Action::RenumberTracks));

  PlaylistMenu::SelectionState playing = OptsFor({PlaylistMenu::FromSong(LocalSong())});
  playing.playing_selected = true;
  EXPECT_EQ("Pause", PlaylistMenu::LabelFor(PlaylistMenu::Action::Play, playing));
  playing.pause_disabled = true;
  EXPECT_FALSE(PlaylistMenu::PlayEnabled(playing));
}
