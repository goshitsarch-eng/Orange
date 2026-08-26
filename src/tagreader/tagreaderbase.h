#ifndef STRAWBERRY_TAGREADERBASE_H
#define STRAWBERRY_TAGREADERBASE_H

#include "core/song.h"

#include <cstdint>
#include <string>
#include <vector>

class TagReaderBase {
 public:
  struct CoverData {
    std::vector<unsigned char> data;
    std::string mime_type;
  };

  virtual ~TagReaderBase() = default;
  virtual bool IsMediaFile(const std::string &filename) const = 0;
  virtual Song ReadFile(const std::string &filename) const = 0;
  virtual bool WriteFile(const Song &song) const = 0;
  virtual CoverData LoadCoverData(const std::string &filename) const = 0;
  virtual bool SaveCover(const std::string &filename, const CoverData &cover) const = 0;
  virtual bool SavePlaycount(const std::string &filename, unsigned playcount) const = 0;
  virtual bool SaveRating(const std::string &filename, float rating) const = 0;
};

#endif
