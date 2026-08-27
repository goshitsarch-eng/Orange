#ifndef STRAWBERRY_JSONCOVERPROVIDER_H
#define STRAWBERRY_JSONCOVERPROVIDER_H

#include "covermanager/coverprovider.h"

#include <string>

class JsonCoverProvider : public CoverProvider {
 public:
  JsonCoverProvider(std::string name, std::string url_template);
  std::string name() const override { return name_; }
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

 protected:
  std::string name_;
  std::string url_template_;
};

#endif
