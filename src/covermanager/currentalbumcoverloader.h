#ifndef STRAWBERRY_CURRENTALBUMCOVERLOADER_H
#define STRAWBERRY_CURRENTALBUMCOVERLOADER_H

#include "core/signal.h"
#include "core/song.h"
#include "covermanager/albumcoverloader.h"

#include <string>
#include <vector>

class CurrentAlbumCoverLoader {
 public:
  explicit CurrentAlbumCoverLoader(AlbumCoverLoader *loader);
  void Load(const Song &song);
  const std::vector<unsigned char> &current() const { return current_; }
  const std::string &current_url() const { return current_url_; }
  Signal<Song, std::vector<unsigned char>> AlbumCoverReady;

 private:
  AlbumCoverLoader *loader_;
  std::vector<unsigned char> current_;
  std::string current_url_;
};

#endif
