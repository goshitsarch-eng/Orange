#ifndef STRAWBERRY_CDDASONGLOADER_H
#define STRAWBERRY_CDDASONGLOADER_H

#include "core/signal.h"
#include "core/song.h"

#include <glib.h>

#include <memory>
#include <string>
#include <vector>

class MusicBrainzClient;
class NetworkAccessManager;

class CddaSongLoader {
 public:
  CddaSongLoader();
  ~CddaSongLoader();

  static SongList Songs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec,
                       const std::string &device_path = {});
  static SongList LoadDevice(const std::string &device_path);
  static SongList LoadDeviceWithFallbacks(const std::string &device_path, const std::vector<std::string> &fallbacks);

  // Heap-friendly async TOC + optional MusicBrainz. Do not call Start from unit tests.
  void Start(const std::string &device_path, NetworkAccessManager *network, const std::vector<std::string> &fallbacks = {});

  Signal<SongList> SongsLoaded;
  Signal<SongList> SongsUpdated;
  Signal<std::string> LoadError;
  Signal<> LoadingFinished;

 private:
  void LoadBlocking();
  void FinishWithError(const std::string &error);
  static gpointer Thread(gpointer data);
  static gboolean IdleLoaded(gpointer data);

  NetworkAccessManager *network_ = nullptr;
  std::string device_path_;
  std::vector<std::string> fallbacks_;
  SongList songs_;
  bool active_ = false;
  std::shared_ptr<bool> alive_;
  std::unique_ptr<MusicBrainzClient> musicbrainz_;
};

#endif
