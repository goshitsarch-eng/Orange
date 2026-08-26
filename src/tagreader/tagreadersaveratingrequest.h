#ifndef STRAWBERRY_TAGREADERSAVERATINGREQUEST_H
#define STRAWBERRY_TAGREADERSAVERATINGREQUEST_H

#include "tagreader/tagreaderrequest.h"

#include <memory>

class TagReaderSaveRatingRequest : public TagReaderRequest {
 public:
  explicit TagReaderSaveRatingRequest(const std::string &filename) : TagReaderRequest(filename) {}
  static std::shared_ptr<TagReaderSaveRatingRequest> Create(const std::string &filename) {
    return std::make_shared<TagReaderSaveRatingRequest>(filename);
  }
  float rating = 0.0f;
};

using TagReaderSaveRatingRequestPtr = std::shared_ptr<TagReaderSaveRatingRequest>;

#endif
