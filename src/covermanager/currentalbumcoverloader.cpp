#include "covermanager/currentalbumcoverloader.h"

#include "core/standardpaths.h"
#include "utilities/fileutils.h"

CurrentAlbumCoverLoader::CurrentAlbumCoverLoader(AlbumCoverLoader *loader) : loader_(loader) {}

void CurrentAlbumCoverLoader::Load(const Song &song) {
  current_ = loader_ ? loader_->LoadData(song) : std::vector<unsigned char>{};
  current_url_ = !song.art_manual().empty() ? song.art_manual() : song.art_automatic();
  if (current_url_.empty() && !current_.empty()) {
    const std::string path = FileUtils::Join(StandardPaths::CoverCacheDir(), "current-albumcover.jpg");
    if (FileUtils::WriteFile(path, std::string(current_.begin(), current_.end()))) {
      current_url_ = FileUtils::UriFromPath(path);
    }
  }
  AlbumCoverReady.Emit(song, current_);
}
