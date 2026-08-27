#ifndef STRAWBERRY_MUSICBRAINZCLIENT_H
#define STRAWBERRY_MUSICBRAINZCLIENT_H

#include "core/network.h"
#include "core/networktimeouts.h"
#include "core/signal.h"
#include "core/song.h"

#include <map>
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
    std::string musicbrainz_recording_id;
    std::string musicbrainz_artist_id;
    std::string musicbrainz_album_id;
    std::string musicbrainz_album_artist_id;
  };
  using ResultList = std::vector<Result>;

  explicit MusicBrainzClient(NetworkAccessManager *network);

  void Start(int id, const std::vector<std::string> &mbid_list);
  void StartDiscId(const std::string &disc_id);
  void Cancel(int id);
  void CancelAll();

  static ResultList ParseResults(const std::string &json);
  static ResultList ParseDiscResults(const std::string &json, const std::string &disc_id);
  static SongList ToSongs(const ResultList &results);

  Signal<int, ResultList, std::string> Finished;
  Signal<std::string, ResultList, std::string> DiscIdFinished;

 private:
  NetworkAccessManager *network_ = nullptr;
  NetworkTimeouts timeouts_;
  std::map<int, int> requests_;
};

#endif
