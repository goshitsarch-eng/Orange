#ifndef STRAWBERRY_SONGPLAYLISTITEM_H
#define STRAWBERRY_SONGPLAYLISTITEM_H

#include "playlist/playlistitem.h"

class SongPlaylistItem : public PlaylistItem {
 public:
  explicit SongPlaylistItem(Song::Source source, const std::string &uuid = {});
  explicit SongPlaylistItem(const Song &song);

  Song OriginalMetadata() const override { return song_; }
  std::string OriginalUrl() const override { return song_.url(); }
  void SetOriginalMetadata(const Song &song) override { song_ = song; }
  void SetArtManual(const std::string &cover_url) override;
  bool IsLocalCollectionItem() const override { return song_.source() == Song::Source::Collection && song_.id() > 0; }

 protected:
  Song DatabaseSongMetadata() const override { return song_; }

 private:
  Song song_;
};

#endif
