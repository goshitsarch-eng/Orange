#include "playlist/playlistitem.h"

#include "playlist/songplaylistitem.h"
#include "playlist/streamplaylistitem.h"

#include <glib.h>

namespace {

bool IsStreamSource(Song::Source source) {
  return source == Song::Source::Stream || source == Song::Source::Tidal || source == Song::Source::Subsonic ||
         source == Song::Source::Qobuz || source == Song::Source::Spotify || source == Song::Source::SomaFM ||
         source == Song::Source::RadioParadise || source == Song::Source::RadioBrowser;
}

std::string NewUuid() {
  char *uuid = g_uuid_string_random();
  std::string text = uuid ? uuid : "";
  g_free(uuid);
  return text;
}

}  // namespace

PlaylistItem::PlaylistItem(Song::Source source, const std::string &uuid) : source_(source), uuid_(uuid) {
  if (uuid_.empty()) {
    uuid_ = NewUuid();
    uuid_generated_ = true;
  }
}

PlaylistItemPtr PlaylistItem::NewFromSource(Song::Source source, const std::string &uuid) {
  if (IsStreamSource(source)) {
    return std::make_shared<StreamPlaylistItem>(source, uuid);
  }
  return std::make_shared<SongPlaylistItem>(source, uuid);
}

PlaylistItemPtr PlaylistItem::NewFromSong(const Song &song) {
  if (IsStreamSource(song.source())) {
    return std::make_shared<StreamPlaylistItem>(song);
  }
  return std::make_shared<SongPlaylistItem>(song);
}

Song PlaylistItem::EffectiveMetadata() const { return HasStreamMetadata() ? stream_song_ : OriginalMetadata(); }

std::string PlaylistItem::EffectiveUrl() const {
  if (HasStreamMetadata() && !stream_song_.stream_url().empty()) {
    return stream_song_.stream_url();
  }
  if (HasStreamMetadata() && !stream_song_.url().empty()) {
    return stream_song_.url();
  }
  return OriginalUrl();
}

void PlaylistItem::SetStreamMetadata(const Song &song) { stream_song_ = song; }

void PlaylistItem::UpdateStreamMetadata(const Song &song) {
  if (!stream_song_.is_valid()) {
    stream_song_ = song;
    return;
  }
  if (!song.title().empty()) {
    stream_song_.set_title(song.title());
  }
  if (!song.artist().empty()) {
    stream_song_.set_artist(song.artist());
  }
  if (!song.album().empty()) {
    stream_song_.set_album(song.album());
  }
  if (song.length_nanosec() > 0) {
    stream_song_.set_length_nanosec(song.length_nanosec());
  }
  if (!song.stream_url().empty()) {
    stream_song_.set_stream_url(song.stream_url());
  }
}

void PlaylistItem::ClearStreamMetadata() { stream_song_ = Song(); }

PlaylistItemSaveData PlaylistItem::CreateSaveData() const {
  PlaylistItemSaveData data(DatabaseSongMetadata(), uuid_);
  data.source = source();
  return data;
}
