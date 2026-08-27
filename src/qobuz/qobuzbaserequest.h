#ifndef STRAWBERRY_QOBUZBASEREQUEST_H
#define STRAWBERRY_QOBUZBASEREQUEST_H

#include "core/jsonbaserequest.h"

class QobuzBaseRequest : public JsonBaseRequest {
 public:
  explicit QobuzBaseRequest(NetworkAccessManager *network) : JsonBaseRequest(network) {}
  std::string service_name() const override { return "Qobuz"; }
};

#endif
