#ifndef STRAWBERRY_TAGREADERLOADCOVERDATAREQUEST_H
#define STRAWBERRY_TAGREADERLOADCOVERDATAREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderLoadCoverDataRequest : public TagReaderRequest {
 public:
  explicit TagReaderLoadCoverDataRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderLoadCoverDataRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderLoadCoverDataRequest>(filename);
  }
};

using TagReaderLoadCoverDataRequestPtr = std::shared_ptr<TagReaderLoadCoverDataRequest>;

#endif
