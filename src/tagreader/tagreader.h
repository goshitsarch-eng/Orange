#ifndef STRAWBERRY_TAGREADER_H
#define STRAWBERRY_TAGREADER_H

#include "core/song.h"
#include "tagreader/streamtagreader.h"
#include "tagreader/tagreaderbase.h"

#include <cstdint>
#include <string>
#include <vector>

class TagReader : public TagReaderBase {
 public:
  using CoverData = TagReaderBase::CoverData;

  bool IsMediaFile(const std::string &filename) const override;
  Song ReadFile(const std::string &filename) const override;
  Song ReadStream(const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime, const std::string &token_type = {},
                  const std::string &access_token = {}) const;
  Song ReadStream(StreamTagReader *stream, const std::string &url, const std::string &filename, uint64_t size, uint64_t mtime) const;
  bool WriteFile(const Song &song) const override;
  CoverData LoadCoverData(const std::string &filename) const override;
  bool SaveCover(const std::string &filename, const CoverData &cover) const override;
  bool ClearCover(const std::string &filename) const;
  bool SavePlaycount(const std::string &filename, unsigned playcount) const override;
  bool SaveRating(const std::string &filename, float rating) const override;

 private:
  void ApplyFileInfo(Song *song, const std::string &filename) const;
};

#endif
