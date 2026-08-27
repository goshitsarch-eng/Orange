#ifndef STRAWBERRY_TAGREADERSAVEPLAYCOUNTREQUEST_H
#define STRAWBERRY_TAGREADERSAVEPLAYCOUNTREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderSavePlaycountRequest : public TagReaderRequest {
 public:
  explicit TagReaderSavePlaycountRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderSavePlaycountRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderSavePlaycountRequest>(filename);
  }
  unsigned playcount = 0;
};

using TagReaderSavePlaycountRequestPtr = std::shared_ptr<TagReaderSavePlaycountRequest>;

#endif
