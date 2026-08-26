#include "covermanager/currentalbumcoverloader.h"

CurrentAlbumCoverLoader::CurrentAlbumCoverLoader(AlbumCoverLoader *loader) : loader_(loader) {}

void CurrentAlbumCoverLoader::Load(const Song &song) {
  current_ = loader_ ? loader_->LoadData(song) : std::vector<unsigned char>{};
  AlbumCoverReady.Emit(song, current_);
}
