#include "tagreader/albumcovertagdata.h"

std::string AlbumCoverTagData::GuessMimeType(const std::vector<unsigned char> &data) {
  if (data.size() >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
    return "image/jpeg";
  }
  if (data.size() >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G') {
    return "image/png";
  }
  if (data.size() >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F') {
    return "image/gif";
  }
  if (data.size() >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' && data[8] == 'W' && data[9] == 'E' &&
      data[10] == 'B' && data[11] == 'P') {
    return "image/webp";
  }
  return "image/jpeg";
}

AlbumCoverTagData AlbumCoverTagData::FromBytes(const std::vector<unsigned char> &data, const std::string &mimetype) {
  AlbumCoverTagData cover(data, mimetype.empty() ? GuessMimeType(data) : mimetype);
  return cover;
}
