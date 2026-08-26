#ifndef STRAWBERRY_SUBSONICBASEREQUEST_H
#define STRAWBERRY_SUBSONICBASEREQUEST_H

#include "core/jsonbaserequest.h"
#include "subsonic/subsonicrequest.h"

class SubsonicBaseRequest : public JsonBaseRequest {
 public:
  explicit SubsonicBaseRequest(NetworkAccessManager *network) : JsonBaseRequest(network) {}
  std::string service_name() const override { return "Subsonic"; }
  static SubsonicRequest::Type TypeFromSearch(StreamingService::SearchType type) { return SubsonicRequest::FromSearchType(type); }
};

#endif
