#ifndef STRAWBERRY_SONGLOADERINSERTER_H
#define STRAWBERRY_SONGLOADERINSERTER_H

#include "core/song.h"

#include <string>
#include <vector>

class Playlist;
class TagReader;

class SongLoaderInserter {
 public:
  explicit SongLoaderInserter(TagReader *tagreader);

  SongList Load(const std::vector<std::string> &urls) const;
  int Insert(Playlist *playlist, const std::vector<std::string> &urls, int row = -1) const;

 private:
  TagReader *tagreader_ = nullptr;
};

#endif
