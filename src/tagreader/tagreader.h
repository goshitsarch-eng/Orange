#ifndef STRAWBERRY_TAGREADER_H
#define STRAWBERRY_TAGREADER_H

#include "core/song.h"

#include <string>
#include <vector>

class TagReader {
 public:
  struct CoverData {
    std::vector<unsigned char> data;
    std::string mime_type;
  };

  bool IsMediaFile(const std::string &filename) const;
  Song ReadFile(const std::string &filename) const;
  bool WriteFile(const Song &song) const;
  CoverData LoadCoverData(const std::string &filename) const;
  bool SaveCover(const std::string &filename, const CoverData &cover) const;
  bool SavePlaycount(const std::string &filename, unsigned playcount) const;
  bool SaveRating(const std::string &filename, float rating) const;

 private:
  void ApplyFileInfo(Song *song, const std::string &filename) const;
};

#endif
