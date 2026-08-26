#ifndef STRAWBERRY_TAGFETCHER_H
#define STRAWBERRY_TAGFETCHER_H
#include "core/network.h"
#include "core/song.h"
#include "core/signal.h"
#include <string>
#include <vector>
class TagFetcher {
 public:
  explicit TagFetcher(NetworkAccessManager *network);
  void Fetch(const Song &song);
  Signal<SongList> Results;
 private:
  NetworkAccessManager *network_;
};
#endif
