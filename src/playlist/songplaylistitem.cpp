#include "playlist/songplaylistitem.h"

SongPlaylistItem::SongPlaylistItem(Song::Source source, const std::string &uuid) : PlaylistItem(source, uuid) {
  song_.set_source(source);
}

SongPlaylistItem::SongPlaylistItem(const Song &song) : PlaylistItem(song.source() == Song::Source::Unknown ? Song::Source::LocalFile : song.source()), song_(song) {
  if (song_.source() == Song::Source::Unknown) {
    song_.set_source(Song::Source::LocalFile);
  }
}

void SongPlaylistItem::SetArtManual(const std::string &cover_url) { song_.set_art_manual(cover_url); }
