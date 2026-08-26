#ifndef STRAWBERRY_TIDALBASEREQUEST_H
#define STRAWBERRY_TIDALBASEREQUEST_H

#include "core/jsonbaserequest.h"

class TidalBaseRequest : public JsonBaseRequest {
 public:
  explicit TidalBaseRequest(NetworkAccessManager *network) : JsonBaseRequest(network) {}
  std::string service_name() const override { return "Tidal"; }
};

#endif
