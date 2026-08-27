#ifndef STRAWBERRY_SPOTIFYBASEREQUEST_H
#define STRAWBERRY_SPOTIFYBASEREQUEST_H

#include "core/jsonbaserequest.h"

class SpotifyBaseRequest : public JsonBaseRequest {
 public:
  explicit SpotifyBaseRequest(NetworkAccessManager *network) : JsonBaseRequest(network) {}
  std::string service_name() const override { return "Spotify"; }
};

#endif
