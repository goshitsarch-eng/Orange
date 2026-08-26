#ifndef STRAWBERRY_LYRICSPROVIDERS_H
#define STRAWBERRY_LYRICSPROVIDERS_H

#include "core/network.h"
#include "core/song.h"
#include "lyrics/lyricsprovider.h"

#include <memory>
#include <string>
#include <vector>

class LyricsProviders {
 public:
  explicit LyricsProviders(NetworkAccessManager *network);
  void ReloadSettings();
  void Move(int index, int delta);
  void SaveOrder();
  void Fetch(const Song &song, LyricsProvider::Callback callback);
  std::vector<LyricsProvider *> All() const;
  NetworkAccessManager *network() const { return network_; }

 private:
  void FetchFromIndex(const Song &song, size_t index, LyricsProvider::Callback callback);

  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<LyricsProvider>> providers_;
};
#endif
