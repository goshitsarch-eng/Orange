#ifndef STRAWBERRY_TAGREADERREADSTREAMREQUEST_H
#define STRAWBERRY_TAGREADERREADSTREAMREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <cstdint>
#include <memory>

class TagReaderReadStreamRequest : public TagReaderRequest {
 public:
  TagReaderReadStreamRequest(const std::string &url, const std::string &filename) : TagReaderRequest(url, filename) {}
  static std::shared_ptr<TagReaderReadStreamRequest> Create(const std::string &url, const std::string &filename) {
    return std::make_shared<TagReaderReadStreamRequest>(url, filename);
  }
  uint64_t size = 0;
  uint64_t mtime = 0;
  std::string token_type;
  std::string access_token;
};

using TagReaderReadStreamRequestPtr = std::shared_ptr<TagReaderReadStreamRequest>;

#endif
