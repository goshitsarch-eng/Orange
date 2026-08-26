#ifndef STRAWBERRY_JSONBASEREQUEST_H
#define STRAWBERRY_JSONBASEREQUEST_H

#include "core/httpbaserequest.h"

class JsonBaseRequest : public HttpBaseRequest {
 public:
  using HttpBaseRequest::HttpBaseRequest;
  static bool IsObject(const std::string &json);
  static std::string StringValue(const std::string &json, const std::string &key);
};

#endif
