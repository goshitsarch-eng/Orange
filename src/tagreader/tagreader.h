#ifndef STRAWBERRY_TAGREADER_H
#define STRAWBERRY_TAGREADER_H

#include "core/song.h"
#include "tagreader/streamtagreader.h"

#include <cstdint>
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
  Song ReadStream(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime, const std::string &token_type = {},
                  const std::string &access_token = {}) const;
  Song ReadStream(StreamTagReader *stream, const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime) const;
  bool WriteFile(const Song &song) const;
  CoverData LoadCoverData(const std::string &filename) const;
  bool SaveCover(const std::string &filename, const CoverData &cover) const;
  bool SavePlaycount(const std::string &filename, unsigned playcount) const;
  bool SaveRating(const std::string &filename, float rating) const;

 private:
  void ApplyFileInfo(Song *song, const std::string &filename) const;
};

#endif
