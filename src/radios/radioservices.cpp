#include "radios/radioservices.h"

#include "core/logging.h"
#include "core/settings.h"
#include "playlistparsers/playlistparser.h"
#include "radios/radiobrowserservice.h"
#include "radios/radioparadiseservice.h"
#include "radios/somafmservice.h"

#include <algorithm>
#include <memory>

RadioServices::RadioServices(Database *database, NetworkAccessManager *network) : database_(database), network_(network) {
  Reload();
}

void RadioServices::Reload() {
  channels_.clear();
  if (!database_) {
    return;
  }
  SqlQuery query(database_, "SELECT source, name, url, thumbnail_url FROM radio_channels");
  while (query.Step()) {
    RadioChannel channel;
    channel.source = static_cast<Song::Source>(query.ColumnInt(0));
    channel.name = query.ColumnText(1);
    channel.url = query.ColumnText(2);
    channel.thumbnail_url = query.ColumnText(3);
    channels_.push_back(channel);
  }
}

void RadioServices::Persist(const RadioChannel &channel) {
  if (!database_) {
    return;
  }
  SqlQuery query(database_, "INSERT INTO radio_channels (source, name, url, thumbnail_url) VALUES (?, ?, ?, ?)");
  query.Bind(1, static_cast<int>(channel.source));
  query.Bind(2, channel.name);
  query.Bind(3, channel.url);
  query.Bind(4, channel.thumbnail_url);
  query.Exec();
}

void RadioServices::RemoveSource(Song::Source source, bool persist) {
  channels_.erase(std::remove_if(channels_.begin(), channels_.end(),
                                 [source](const RadioChannel &channel) { return channel.source == source; }),
                  channels_.end());
  if (persist && database_) {
    SqlQuery query(database_, "DELETE FROM radio_channels WHERE source = ?");
    query.Bind(1, static_cast<int>(source));
    query.Exec();
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
  search_results_.clear();
  if (!network_ || query.empty()) {
    NotifyUpdated();
    return;
  }
  Settings settings;
  settings.BeginGroup("RadioBrowser");
  const std::string url = RadioBrowserService::SearchUrl(settings.Value("server", RadioBrowserService::DefaultServer()), query,
                                                         settings.Value("country"), settings.BoolValue("hidebroken", true));
  network_->Get(url, [this](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      NotifyUpdated();
      return;
    }
    search_results_ = RadioBrowserService::ParseStations(response.body);
    NotifyUpdated();
  });
}
