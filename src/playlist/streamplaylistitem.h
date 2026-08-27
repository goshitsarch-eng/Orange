#ifndef STRAWBERRY_STREAMPLAYLISTITEM_H
#define STRAWBERRY_STREAMPLAYLISTITEM_H

#include "playlist/playlistitem.h"

class StreamPlaylistItem : public PlaylistItem {
 public:
  explicit StreamPlaylistItem(Song::Source source, const std::string &uuid = {});
  explicit StreamPlaylistItem(const Song &song);

  Song OriginalMetadata() const override { return song_; }
  std::string OriginalUrl() const override { return song_.url(); }
  void SetOriginalMetadata(const Song &song) override { song_ = song; }
  void SetArtManual(const std::string &cover_url) override;
  Option options() const override;

 protected:
  Song DatabaseSongMetadata() const override { return song_; }

 private:
  void InitMetadata();

  Song song_;
};

#endif
