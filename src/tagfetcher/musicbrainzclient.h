#ifndef STRAWBERRY_MUSICBRAINZCLIENT_H
#define STRAWBERRY_MUSICBRAINZCLIENT_H

#include "core/network.h"
#include "core/signal.h"
#include "core/song.h"

#include <string>
#include <vector>

class MusicBrainzClient {
 public:
  struct Result {
    std::string title;
    std::string artist;
    std::string album;
    std::string album_artist;
    int track = 0;
    int year = -1;
    int duration_msec = 0;
  };
  using ResultList = std::vector<Result>;

  explicit MusicBrainzClient(NetworkAccessManager *network);

  void Start(int id, const std::vector<std::string> &mbid_list);
  void Cancel(int id);
  void CancelAll();

  static ResultList ParseResults(const std::string &json);
  static SongList ToSongs(const ResultList &results);

  Signal<int, ResultList, std::string> Finished;

 private:
  NetworkAccessManager *network_ = nullptr;
};

#endif
