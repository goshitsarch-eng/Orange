#ifndef STRAWBERRY_ALBUMCOVERTAGDATA_H
#define STRAWBERRY_ALBUMCOVERTAGDATA_H

#include <string>
#include <vector>

class AlbumCoverTagData {
 public:
  AlbumCoverTagData() = default;
  explicit AlbumCoverTagData(const std::vector<unsigned char> &data, const std::string &mimetype = {}) : data(data), mimetype(mimetype) {}

  std::vector<unsigned char> data;
  std::string mimetype;
  std::string error;
};

#endif
