#include "covermanager/albumcoverloader.h"

#include "tagreader/tagreader.h"
#include "utilities/fileutils.h"

AlbumCoverLoader::AlbumCoverLoader(TagReader *tagreader) : tagreader_(tagreader) {}

std::string AlbumCoverLoader::LoadPath(const Song &song) const {
  if (!song.art_manual().empty()) {
    return song.art_manual();
  }
  if (!song.art_automatic().empty()) {
    return song.art_automatic();
  }
  const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
  for (const char *name : {"cover.jpg", "cover.png", "folder.jpg", "front.jpg", "album.jpg"}) {
    const std::string candidate = FileUtils::Join(dir, name);
    if (FileUtils::Exists(candidate)) {
      return FileUtils::UriFromPath(candidate);
    }
  }
  return {};
}

std::vector<unsigned char> AlbumCoverLoader::LoadData(const Song &song) const {
  return LoadData(song, AlbumCoverLoaderOptions::LoadTypes());
}

std::vector<unsigned char> AlbumCoverLoader::LoadData(const Song &song, const AlbumCoverLoaderOptions::Types &types) const {
  for (const AlbumCoverLoaderOptions::Type type : types) {
    switch (type) {
      case AlbumCoverLoaderOptions::Type::Unset:
        if (song.art_unset()) {
          return {};
        }
        break;
      case AlbumCoverLoaderOptions::Type::Embedded:
        if (tagreader_ && song.art_embedded()) {
          auto cover = tagreader_->LoadCoverData(FileUtils::PathFromUri(song.url()));
          if (!cover.data.empty()) {
            return cover.data;
          }
        }
        break;
      case AlbumCoverLoaderOptions::Type::Manual:
        if (!song.art_manual().empty()) {
          const std::string path = FileUtils::PathFromUri(song.art_manual());
          if (FileUtils::Exists(path)) {
            const std::string data = FileUtils::ReadFile(path);
            return std::vector<unsigned char>(data.begin(), data.end());
          }
        }
        break;
      case AlbumCoverLoaderOptions::Type::Automatic:
        if (!song.art_automatic().empty()) {
          const std::string path = FileUtils::PathFromUri(song.art_automatic());
          if (FileUtils::Exists(path)) {
            const std::string data = FileUtils::ReadFile(path);
            return std::vector<unsigned char>(data.begin(), data.end());
          }
        }
        break;
    }
  }
  const std::string path = FileUtils::PathFromUri(LoadPath(song));
  if (!path.empty() && FileUtils::Exists(path)) {
    const std::string data = FileUtils::ReadFile(path);
    return std::vector<unsigned char>(data.begin(), data.end());
  }
  return {};
}
