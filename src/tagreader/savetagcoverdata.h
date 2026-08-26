#ifndef STRAWBERRY_SAVETAGCOVERDATA_H
#define STRAWBERRY_SAVETAGCOVERDATA_H

#include <string>
#include <vector>

class SaveTagCoverData {
 public:
  SaveTagCoverData() = default;
  SaveTagCoverData(const std::string &cover_filename, const std::vector<unsigned char> &cover_data = {}, const std::string &cover_mimetype = {})
      : cover_filename(cover_filename), cover_data(cover_data), cover_mimetype(cover_mimetype) {}
  explicit SaveTagCoverData(const std::vector<unsigned char> &cover_data, const std::string &cover_mimetype = {})
      : cover_data(cover_data), cover_mimetype(cover_mimetype) {}

  std::string cover_filename;
  std::vector<unsigned char> cover_data;
  std::string cover_mimetype;
};

#endif
