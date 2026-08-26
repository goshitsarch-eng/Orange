#ifndef STRAWBERRY_TAGREADERREQUEST_H
#define STRAWBERRY_TAGREADERREQUEST_H

#include "tagreader/tagreaderreply.h"

#include <memory>
#include <string>

class TagReaderRequest {
 public:
  explicit TagReaderRequest(const std::string &filename) : filename(filename) {}
  TagReaderRequest(const std::string &url, const std::string &filename) : filename(filename), url(url) {}
  virtual ~TagReaderRequest() = default;

  std::string filename;
  std::string url;
  std::shared_ptr<TagReaderReply> reply;
};

using TagReaderRequestPtr = std::shared_ptr<TagReaderRequest>;

#endif
