#include "playlist/playlist.h"
#include "playlist/playlistfilter.h"
#include "playlist/playlistitem.h"
#include "playlist/playlistitemmimedata.h"
#include "playlist/playlistitemsavedata.h"
#include "playlist/playlistsequence.h"
#include "playlist/playlistundocommandbase.h"
#include "playlist/songplaylistitem.h"
#include "playlist/streamplaylistitem.h"

#include <gtest/gtest.h>

TEST(PlaylistItem, NewFromSongAndStreamMetadata) {
  Song song(Song::Source::Collection);
  song.set_id(42);
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_album("Dummy");
  song.set_url("file:///tmp/roads.flac");
  song.set_valid(true);
  PlaylistItemPtr item = PlaylistItem::NewFromSong(song);
  ASSERT_TRUE(item);
  EXPECT_EQ(Song::Source::Collection, item->source());
  EXPECT_EQ("Roads", item->OriginalMetadata().title());
  EXPECT_EQ("file:///tmp/roads.flac", item->OriginalUrl());
  EXPECT_TRUE(item->IsLocalCollectionItem());
  EXPECT_FALSE(item->uuid().empty());

  Song stream;
  stream.set_title("Roads (stream)");
  stream.set_url("https://example.test/roads");
  stream.set_stream_url("https://cdn.example.test/roads");
  stream.set_length_nanosec(123);
  stream.set_valid(true);
  item->SetStreamMetadata(stream);
  EXPECT_TRUE(item->HasStreamMetadata());
  EXPECT_EQ("Roads (stream)", item->EffectiveMetadata().title());
  EXPECT_EQ("https://cdn.example.test/roads", item->EffectiveUrl());
  item->ClearStreamMetadata();
  EXPECT_EQ("Roads", item->EffectiveMetadata().title());

  const PlaylistItemSaveData saved = item->CreateSaveData();
  EXPECT_EQ(Song::Source::Collection, saved.source);
  EXPECT_EQ(42, saved.collection_id);
  EXPECT_EQ(item->uuid(), saved.uuid);
}

TEST(StreamPlaylistItem, DisablesSeek) {
  Song song(Song::Source::Tidal);
  song.set_title("Roads");
  song.set_url("tidal://track/1");
  PlaylistItemPtr item = PlaylistItem::NewFromSong(song);
  ASSERT_TRUE(item);
  EXPECT_EQ(Song::Source::Tidal, item->source());
  EXPECT_EQ(PlaylistItem::Option::SeekDisabled, item->options());
}

TEST(PlaylistItemMimeData, CollectsEffectiveSongs) {
  Song song;
  song.set_title("Roads");
  song.set_url("file:///tmp/roads.flac");
  song.set_valid(true);
  PlaylistItemMimeData mime(PlaylistItem::NewFromSong(song));
  ASSERT_EQ(1u, mime.items.size());
  ASSERT_EQ(1u, mime.Songs().size());
  EXPECT_EQ("Roads", mime.Songs().front().title());
}

TEST(PlaylistFilter, AcceptsFilterParserRows) {
  Song match;
  match.set_title("Roads");
  match.set_artist("Portishead");
  match.set_valid(true);
  Song other;
  other.set_title("Helplessness Blues");
  other.set_artist("Fleet Foxes");
  other.set_valid(true);
  PlaylistFilter filter;
  filter.SetFilterString("artist:Portishead");
  EXPECT_TRUE(filter.Accepts(match));
  EXPECT_FALSE(filter.Accepts(other));
  EXPECT_TRUE(filter.filterAcceptsRow(0, {match, other}));
  EXPECT_FALSE(filter.filterAcceptsRow(1, {match, other}));
  EXPECT_EQ(1u, filter.FilterSongs({match, other}).size());
  ASSERT_EQ(1u, filter.VisibleRows({match, other}).size());
  EXPECT_EQ(0, filter.VisibleRows({match, other}).front());
}

TEST(PlaylistSequence, CyclesOriginalRepeatAndShuffleModes) {
  PlaylistSequence sequence;
  sequence.SetRepeatMode(PlaylistSequence::RepeatMode::Off);
  sequence.SetShuffleMode(PlaylistSequence::ShuffleMode::Off);
  sequence.CycleRepeatMode();
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, sequence.repeat_mode());
  sequence.CycleRepeatMode();
  EXPECT_EQ(PlaylistSequence::RepeatMode::Album, sequence.repeat_mode());
  sequence.CycleShuffleMode();
  EXPECT_EQ(PlaylistSequence::ShuffleMode::All, sequence.shuffle_mode());
  EXPECT_STREQ("Repeat album", PlaylistSequence::RepeatLabel(PlaylistSequence::RepeatMode::Album));
  EXPECT_STREQ("Shuffle all", PlaylistSequence::ShuffleLabel(PlaylistSequence::ShuffleMode::All));
}

TEST(PlaylistSequence, MenuModesMatchQtOrder) {
  const auto repeat = PlaylistSequence::RepeatModes();
  ASSERT_EQ(6u, repeat.size());
  EXPECT_EQ(PlaylistSequence::RepeatMode::Off, repeat[0]);
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, repeat[1]);
  EXPECT_EQ(PlaylistSequence::RepeatMode::Album, repeat[2]);
  EXPECT_EQ(PlaylistSequence::RepeatMode::Playlist, repeat[3]);
  EXPECT_EQ(PlaylistSequence::RepeatMode::OneByOne, repeat[4]);
  EXPECT_EQ(PlaylistSequence::RepeatMode::Intro, repeat[5]);
  const auto shuffle = PlaylistSequence::ShuffleModes();
  ASSERT_EQ(5u, shuffle.size());
  EXPECT_EQ(PlaylistSequence::ShuffleMode::Off, shuffle[0]);
  EXPECT_EQ(PlaylistSequence::ShuffleMode::All, shuffle[1]);
  EXPECT_EQ(PlaylistSequence::ShuffleMode::InsideAlbum, shuffle[2]);
  EXPECT_EQ(PlaylistSequence::ShuffleMode::Albums, shuffle[3]);
  EXPECT_EQ(PlaylistSequence::ShuffleMode::Grouping, shuffle[4]);
  EXPECT_FALSE(PlaylistSequence::RepeatActive(PlaylistSequence::RepeatMode::Off));
  EXPECT_TRUE(PlaylistSequence::RepeatActive(PlaylistSequence::RepeatMode::Track));
  EXPECT_FALSE(PlaylistSequence::ShuffleActive(PlaylistSequence::ShuffleMode::Off));
  EXPECT_TRUE(PlaylistSequence::ShuffleActive(PlaylistSequence::ShuffleMode::All));
  EXPECT_STREQ("Don't repeat", PlaylistSequence::RepeatLabel(PlaylistSequence::RepeatMode::Off));
  EXPECT_STREQ("Stop after every track", PlaylistSequence::RepeatLabel(PlaylistSequence::RepeatMode::OneByOne));
  EXPECT_STREQ("Intro tracks", PlaylistSequence::RepeatLabel(PlaylistSequence::RepeatMode::Intro));
  EXPECT_STREQ("Don't shuffle", PlaylistSequence::ShuffleLabel(PlaylistSequence::ShuffleMode::Off));
  EXPECT_STREQ("Shuffle tracks in this album", PlaylistSequence::ShuffleLabel(PlaylistSequence::ShuffleMode::InsideAlbum));
  EXPECT_STREQ("Repeat", PlaylistSequence::RepeatButtonTooltip());
  EXPECT_STREQ("Shuffle", PlaylistSequence::ShuffleButtonTooltip());
}

TEST(PlaylistUndoCommand, StoresInsertRemoveMoveSort) {
  Song song;
  song.set_title("Roads");
  PlaylistUndoCommandInsertItems insert(3, {song});
  EXPECT_EQ(PlaylistUndoCommandBase::Type::InsertItems, insert.type());
  EXPECT_EQ(3, insert.row());
  EXPECT_EQ("Insert items", insert.text());
  PlaylistUndoCommandRemoveItems remove({1, 2}, {song});
  EXPECT_EQ(2u, remove.rows().size());
  PlaylistUndoCommandMoveItems move(1, 4);
  EXPECT_EQ(1, move.from());
  EXPECT_EQ(4, move.to());
  PlaylistUndoCommandSortItems sort(2, true);
  EXPECT_TRUE(sort.descending());
  EXPECT_EQ("Shuffle items", PlaylistUndoCommandShuffleItems().text());
}

TEST(Playlist, RepeatAlbumStaysInsideAlbum) {
  Playlist playlist;
  Song a;
  a.set_title("A");
  a.set_album("Dummy");
  a.set_url("file:///a");
  a.set_valid(true);
  Song b;
  b.set_title("B");
  b.set_album("Dummy");
  b.set_url("file:///b");
  b.set_valid(true);
  Song c;
  c.set_title("C");
  c.set_album("Other");
  c.set_url("file:///c");
  c.set_valid(true);
  playlist.AppendSongs({a, b, c});
  playlist.set_current_row(0);
  playlist.SetRepeatMode(PlaylistSequence::RepeatMode::Album);
  EXPECT_EQ(1, playlist.PeekNextRow());
  playlist.set_current_row(1);
  EXPECT_EQ(0, playlist.PeekNextRow());
}
