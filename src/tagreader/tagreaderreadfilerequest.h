#ifndef STRAWBERRY_TAGREADERREADFILEREQUEST_H
#define STRAWBERRY_TAGREADERREADFILEREQUEST_H

#include "core/song.h"
#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderReadFileRequest : public TagReaderRequest {
 public:
  explicit TagReaderReadFileRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderReadFileRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderReadFileRequest>(filename);
  }
  Song song;
};

using TagReaderReadFileRequestPtr = std::shared_ptr<TagReaderReadFileRequest>;

#endif
