#include "radios/radioservices.h"

#include "constants/radiobrowsersettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "playlistparsers/playlistparser.h"
#include "radios/radiobackend.h"
#include "radios/radiobrowsersearchopts.h"
#include "radios/radiobrowserservice.h"
#include "radios/radioparadiseservice.h"
#include "radios/somafmservice.h"

#include <algorithm>
#include <memory>

RadioServices::RadioServices(Database *database, NetworkAccessManager *network)
    : network_(network), backend_(std::make_unique<RadioBackend>(database)) {
  Reload();
}

void RadioServices::Reload() {
  channels_.clear();
  if (backend_) {
    channels_ = backend_->Load();
  }
}

void RadioServices::Persist(const RadioChannel &channel) {
  if (backend_) {
    backend_->Save(channel);
  }
}

void RadioServices::RemoveSource(Song::Source source, bool persist) {
  channels_.erase(std::remove_if(channels_.begin(), channels_.end(),
                                 [source](const RadioChannel &channel) { return channel.source == source; }),
                  channels_.end());
  if (persist && backend_) {
    backend_->RemoveSource(source);
  }
}

void RadioServices::NotifyUpdated() {
  if (updated_) {
    updated_();
  }
}

void RadioServices::AddCustomStream(const std::string &name, const std::string &url) {
  RadioChannel channel;
  channel.name = name;
  channel.url = url;
  channel.source = Song::Source::Stream;
  channels_.push_back(channel);
  Persist(channel);
  NotifyUpdated();
}

void RadioServices::ResolveSomaFMPlaylists(std::vector<RadioChannel> channels) {
  if (channels.empty()) {
    NotifyUpdated();
    return;
  }
  if (!network_) {
    for (const RadioChannel &channel : channels) {
      channels_.push_back(channel);
      Persist(channel);
    }
    NotifyUpdated();
    return;
  }
  auto remaining = std::make_shared<int>(static_cast<int>(channels.size()));
  auto results = std::make_shared<std::vector<RadioChannel>>(std::move(channels));
  for (size_t i = 0; i < results->size(); ++i) {
    const std::string playlist_url = (*results)[i].url;
    network_->Get(playlist_url, [this, remaining, results, i](const NetworkAccessManager::Response &response) {
      if (response.ok()) {
        PlaylistParser parser;
        const SongList songs = parser.LoadFromData(response.body, (*results)[i].url);
        if (!songs.empty() && !songs.front().url().empty()) {
          (*results)[i].url = songs.front().url();
        }
      }
      if (--(*remaining) == 0) {
        for (const RadioChannel &channel : *results) {
          channels_.push_back(channel);
          Persist(channel);
        }
        NotifyUpdated();
      }
    });
  }
}

void RadioServices::FetchSomaFM() {
  if (!network_) {
    return;
  }
  network_->Get(SomaFMService::kApiChannelsUrl, [this](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      return;
    }
    Settings settings;
    settings.BeginGroup("SomaFM");
    std::vector<RadioChannel> parsed = SomaFMService::ParseChannels(response.body, settings.Value("quality", SomaFMService::kQualityDefault));
    RemoveSource(Song::Source::SomaFM, true);
    if (parsed.empty()) {
      RadioChannel channel;
      channel.name = "SomaFM Groove Salad";
      channel.url = "https://ice1.somafm.com/groovesalad-128-mp3";
      channel.source = Song::Source::SomaFM;
      channels_.push_back(channel);
      Persist(channel);
      NotifyUpdated();
      return;
    }
    ResolveSomaFMPlaylists(std::move(parsed));
  });
}

void RadioServices::FetchRadioParadise() {
  if (!network_) {
    return;
  }
  network_->Get(RadioParadiseService::kApiChannelsUrl, [this](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      return;
    }
    std::vector<RadioChannel> parsed = RadioParadiseService::ParseChannels(response.body);
    RemoveSource(Song::Source::RadioParadise, true);
    if (parsed.empty()) {
      RadioChannel channel;
      channel.name = "Radio Paradise";
      Settings settings;
      settings.BeginGroup("RadioParadise");
      channel.url = "https://stream.radioparadise.com/" + settings.Value("quality", "aac-320");
      channel.source = Song::Source::RadioParadise;
      parsed.push_back(channel);
    }
    for (const RadioChannel &channel : parsed) {
      channels_.push_back(channel);
      Persist(channel);
    }
    NotifyUpdated();
  });
}

void RadioServices::FetchRadioBrowser(const std::string &query) {
  Settings settings;
  settings.BeginGroup(RadioBrowserSettings::kSettingsGroup);
  SearchRadioBrowser(query, settings.Value(RadioBrowserSettings::kDefaultCountry),
                     settings.Value(RadioBrowserSettings::kDefaultSort, RadioBrowserSettings::kDefaultSortDefault),
                     settings.IntValue(RadioBrowserSettings::kSearchLimit, RadioBrowserSettings::kSearchLimitDefault), 0,
                     settings.BoolValue(RadioBrowserSettings::kHideBroken, RadioBrowserSettings::kHideBrokenDefault),
                     [this](const std::vector<RadioChannel> &channels, bool, const std::string &) {
                       search_results_ = channels;
                       NotifyUpdated();
                     });
}

void RadioServices::SearchRadioBrowser(const std::string &query, const std::string &country, const std::string &order, int limit,
                                       int offset, bool hide_broken, SearchDone done) {
  if (!network_) {
    if (done) {
      done({}, false, {});
    }
    return;
  }
  Settings settings;
  settings.BeginGroup(RadioBrowserSettings::kSettingsGroup);
  const std::string url = RadioBrowserService::SearchUrl(settings.Value("server", RadioBrowserService::DefaultServer()), query, country,
                                                         hide_broken, limit, offset, order);
  network_->Get(url, [done, limit](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      if (done) {
        done({}, false, RadioBrowserSearchOpts::SearchFailed(response.error));
      }
      return;
    }
    const RadioBrowserService::StationPage page = RadioBrowserService::ParseStationPage(response.body);
    if (done) {
      done(page.channels, RadioBrowserSearchOpts::HasMore(page.raw_count, limit), {});
    }
  });
}

void RadioServices::FetchCountries(CountriesDone done) {
  if (!network_) {
    if (done) {
      done({});
    }
    return;
  }
  Settings settings;
  settings.BeginGroup(RadioBrowserSettings::kSettingsGroup);
  const std::string url = RadioBrowserService::CountriesUrl(settings.Value("server", RadioBrowserService::DefaultServer()));
  network_->Get(url, [done](const NetworkAccessManager::Response &response) {
    if (done) {
      done(response.ok() ? RadioBrowserService::ParseCountries(response.body) : std::vector<RadioBrowserService::Country>{});
    }
  });
}
