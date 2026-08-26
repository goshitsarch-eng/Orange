#ifndef STRAWBERRY_ORGANIZE_H
#define STRAWBERRY_ORGANIZE_H

#include "core/song.h"
#include "organize/organizeformat.h"

#include <string>
#include <vector>

class Organize {
 public:
  struct Error {
    std::string song;
    std::string message;
  };

  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move);
};

#endif
