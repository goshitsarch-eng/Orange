#ifndef STRAWBERRY_RADIOSERVICES_H
#define STRAWBERRY_RADIOSERVICES_H

#include "core/database.h"
#include "core/network.h"
#include "core/song.h"

#include <functional>
#include <string>
#include <vector>

struct RadioChannel {
  std::string name;
  std::string url;
  std::string thumbnail_url;
  std::string country;
  std::string tags;
  std::string codec;
  Song::Source source = Song::Source::Stream;
};

class RadioServices {
 public:
  RadioServices(Database *database, NetworkAccessManager *network);
  void Reload();
  void FetchSomaFM();
  void FetchRadioParadise();
  void FetchRadioBrowser(const std::string &query);
  void AddCustomStream(const std::string &name, const std::string &url);
  const std::vector<RadioChannel> &channels() const { return channels_; }
  const std::vector<RadioChannel> &search_results() const { return search_results_; }
  using Callback = std::function<void()>;
  void set_updated_callback(Callback cb) { updated_ = std::move(cb); }

 private:
  void Persist(const RadioChannel &channel);
  void RemoveSource(Song::Source source, bool persist);
  void ResolveSomaFMPlaylists(std::vector<RadioChannel> channels);
  void NotifyUpdated();

  Database *database_;
  NetworkAccessManager *network_;
  std::vector<RadioChannel> channels_;
  std::vector<RadioChannel> search_results_;
  Callback updated_;
};

#endif
