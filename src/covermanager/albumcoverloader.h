#ifndef STRAWBERRY_ALBUMCOVERLOADER_H
#define STRAWBERRY_ALBUMCOVERLOADER_H

#include "core/song.h"
#include "covermanager/albumcoverloaderoptions.h"

#include <string>
#include <vector>

class TagReader;

class AlbumCoverLoader {
 public:
  explicit AlbumCoverLoader(TagReader *tagreader);
  std::string LoadPath(const Song &song) const;
  std::vector<unsigned char> LoadData(const Song &song) const;
  std::vector<unsigned char> LoadData(const Song &song, const AlbumCoverLoaderOptions::Types &types) const;

 private:
  TagReader *tagreader_;
};

#endif
