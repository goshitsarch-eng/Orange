#ifndef STRAWBERRY_RADIOSERVICES_H
#define STRAWBERRY_RADIOSERVICES_H

#include "core/database.h"
#include "core/network.h"
#include "core/song.h"
#include "radios/radiochannel.h"
#include "radios/radiobackend.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

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

  NetworkAccessManager *network_;
  std::unique_ptr<RadioBackend> backend_;
  std::vector<RadioChannel> channels_;
  std::vector<RadioChannel> search_results_;
  Callback updated_;
};

#endif
