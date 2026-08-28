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
  struct Job {
    int id = 0;
    Song song;
  };

  explicit TagFetcher(NetworkAccessManager *network);
  ~TagFetcher();
  int Fetch(const Song &song);
  std::vector<int> QueueSongs(const SongList &songs);
  std::vector<int> FetchSongs(const SongList &songs);
  void Start();
  void Cancel();
  int pending() const { return static_cast<int>(queue_.size()) + (running_ ? 1 : 0); }
  bool busy() const { return running_ || !queue_.empty(); }
  int current_id() const { return current_.id; }

  Signal<SongList> Results;
  Signal<int, SongList> SongResults;
  Signal<int, std::string> Progress;
  Signal<int, std::string> Error;
  Signal<> Finished;

 private:
  int Enqueue(const Song &song);
  void StartNext();
  void Complete(int id, SongList results, const std::string &error);
  void FetchByMetadata(const Job &job);
  void FetchByFingerprint(const Job &job);

  NetworkAccessManager *network_;
  std::unique_ptr<AcoustidClient> acoustid_;
  std::unique_ptr<MusicBrainzClient> musicbrainz_;
  std::vector<Job> queue_;
  Job current_;
  bool running_ = false;
  bool cancelled_ = false;
  int next_id_ = 1;
  // Network replies arrive long after this client can be destroyed, so callbacks hold a copy of this flag
  // instead of trusting the pointer they captured.
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

#endif
