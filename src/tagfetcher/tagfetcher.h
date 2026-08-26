#ifndef STRAWBERRY_TAGFETCHER_H
#define STRAWBERRY_TAGFETCHER_H

#include "core/network.h"
#include "core/signal.h"
#include "core/song.h"
#include "tagfetcher/acoustidclient.h"
#include "tagfetcher/musicbrainzclient.h"

#include <memory>
#include <string>
#include <vector>

class TagFetcher {
 public:
  explicit TagFetcher(NetworkAccessManager *network);
  void Fetch(const Song &song);
  Signal<SongList> Results;

 private:
  void FetchByMetadata(const Song &song);
  void FetchByFingerprint(const Song &song);

  NetworkAccessManager *network_;
  std::unique_ptr<AcoustidClient> acoustid_;
  std::unique_ptr<MusicBrainzClient> musicbrainz_;
};

#endif
