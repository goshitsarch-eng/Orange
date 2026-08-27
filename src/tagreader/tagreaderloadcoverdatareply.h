#ifndef STRAWBERRY_TAGREADERLOADCOVERDATAREPLY_H
#define STRAWBERRY_TAGREADERLOADCOVERDATAREPLY_H

#include "tagreader/albumcovertagdata.h"
#include "tagreader/tagreaderreply.h"

#include <memory>
#include <vector>

class TagReaderLoadCoverDataReply : public TagReaderReply {
 public:
  explicit TagReaderLoadCoverDataReply(const std::string &filename) : TagReaderReply(filename) {}

  const std::vector<unsigned char> &data() const { return data_.data; }
  void set_data(const std::vector<unsigned char> &data) { data_.data = data; }
  const AlbumCoverTagData &cover() const { return data_; }
  void set_cover(const AlbumCoverTagData &cover) { data_ = cover; }

  void Finish() override {
    finished_ = true;
    DataFinished.Emit(filename_, data_.data, result_);
    TagReaderReply::Finished.Emit(filename_, result_);
  }

  Signal<std::string, std::vector<unsigned char>, TagReaderResult> DataFinished;

 private:
  AlbumCoverTagData data_;
};

using TagReaderLoadCoverDataReplyPtr = std::shared_ptr<TagReaderLoadCoverDataReply>;

#endif
