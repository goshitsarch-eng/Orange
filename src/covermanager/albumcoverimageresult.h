#ifndef STRAWBERRY_ALBUMCOVERIMAGERESULT_H
#define STRAWBERRY_ALBUMCOVERIMAGERESULT_H

#include <string>
#include <vector>

struct AlbumCoverImageResult {
  std::string image_url;
  std::string mime_type;
  std::vector<unsigned char> image_data;
  int width = 0;
  int height = 0;

  bool is_valid() const { return !image_data.empty() || !image_url.empty(); }
};

#endif
