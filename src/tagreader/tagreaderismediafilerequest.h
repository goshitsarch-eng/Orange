#ifndef STRAWBERRY_TAGREADERISMEDIAFILEREQUEST_H
#define STRAWBERRY_TAGREADERISMEDIAFILEREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderIsMediaFileRequest : public TagReaderRequest {
 public:
  explicit TagReaderIsMediaFileRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderIsMediaFileRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderIsMediaFileRequest>(filename);
  }
};

using TagReaderIsMediaFileRequestPtr = std::shared_ptr<TagReaderIsMediaFileRequest>;

#endif
