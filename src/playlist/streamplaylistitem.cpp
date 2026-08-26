#include "playlist/streamplaylistitem.h"

StreamPlaylistItem::StreamPlaylistItem(Song::Source source, const std::string &uuid) : PlaylistItem(source, uuid) {
  song_.set_source(source);
  InitMetadata();
}

StreamPlaylistItem::StreamPlaylistItem(const Song &song) : PlaylistItem(song.source()), song_(song) { InitMetadata(); }

void StreamPlaylistItem::InitMetadata() {
  if (song_.source() == Song::Source::Unknown) {
    song_.set_source(source_);
  }
  if (song_.filetype() == Song::FileType::Unknown) {
    song_.set_filetype(Song::FileType::Stream);
  }
}

void StreamPlaylistItem::SetArtManual(const std::string &cover_url) { song_.set_art_manual(cover_url); }
