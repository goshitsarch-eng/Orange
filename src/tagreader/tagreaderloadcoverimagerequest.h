#ifndef STRAWBERRY_TAGREADERLOADCOVERIMAGEREQUEST_H
#define STRAWBERRY_TAGREADERLOADCOVERIMAGEREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderLoadCoverImageRequest : public TagReaderRequest {
 public:
  explicit TagReaderLoadCoverImageRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderLoadCoverImageRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderLoadCoverImageRequest>(filename);
  }
};

using TagReaderLoadCoverImageRequestPtr = std::shared_ptr<TagReaderLoadCoverImageRequest>;

#endif
