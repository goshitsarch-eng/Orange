#ifndef STRAWBERRY_QOBUZCOVERPROVIDER_H
#define STRAWBERRY_QOBUZCOVERPROVIDER_H

#include "covermanager/coverproviders.h"

#include <string>
#include <vector>

class QobuzCoverProvider : public CoverProvider {
 public:
  struct SearchResult {
    std::string artist;
    std::string album;
    std::string image_url;
  };

  static const int kLimit;

  std::string name() const override { return "Qobuz"; }
  bool authentication_required() const override { return true; }
  bool authenticated() const override;
  void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) override;
  void Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) override;

  static std::string SearchUrl(const std::string &artist, const std::string &album, const std::string &title);
  static std::vector<SearchResult> ParseResults(const std::string &json);
};

#endif
