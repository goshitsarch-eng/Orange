#ifndef STRAWBERRY_TAGREADERSAVECOVERREQUEST_H
#define STRAWBERRY_TAGREADERSAVECOVERREQUEST_H

#include "tagreader/savetagcoverdata.h"
#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderSaveCoverRequest : public TagReaderRequest {
 public:
  explicit TagReaderSaveCoverRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderSaveCoverRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderSaveCoverRequest>(filename);
  }
  SaveTagCoverData save_tag_cover_data;
};

using TagReaderSaveCoverRequestPtr = std::shared_ptr<TagReaderSaveCoverRequest>;

#endif
