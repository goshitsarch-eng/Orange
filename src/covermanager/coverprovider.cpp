#include "covermanager/coverprovider.h"

#include "covermanager/albumcoverfetchersearch.h"
#include "utilities/jsonutils.h"

void CoverProvider::Search(const Song &song, NetworkAccessManager *network, SearchCallback callback) {
  Fetch(song, network, [this, callback, song](const std::string &data, const std::string &) {
    CoverProviderSearchResults results;
    if (data.empty()) {
      callback(results);
      return;
    }
    CoverProviderSearchResult result = AlbumCoverFetcherSearch::FromHit(name(), song.EffectiveAlbumartist(), song.album(), {});
    if (AlbumCoverFetcherSearch::IsHttpUrl(data)) {
      result.image_url = data;
    } else if (JsonUtils::LooksLikeImage(data)) {
      result.image_data = data;
    } else {
      callback(results);
      return;
    }
    results.push_back(result);
    callback(results);
  });
}
