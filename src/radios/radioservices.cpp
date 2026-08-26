#include "radios/radioservices.h"
#include "core/logging.h"
#include "core/settings.h"

RadioServices::RadioServices(Database *database, NetworkAccessManager *network) : database_(database), network_(network) { Reload(); }

void RadioServices::Reload() {
  channels_.clear();
  if (!database_) return;
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
  if (!database_) return;
  SqlQuery query(database_, "INSERT INTO radio_channels (source, name, url, thumbnail_url) VALUES (?, ?, ?, ?)");
  query.Bind(1, static_cast<int>(channel.source));
  query.Bind(2, channel.name);
  query.Bind(3, channel.url);
  query.Bind(4, channel.thumbnail_url);
  query.Exec();
}

void RadioServices::AddCustomStream(const std::string &name, const std::string &url) {
  RadioChannel channel;
  channel.name = name;
  channel.url = url;
  channel.source = Song::Source::Stream;
  channels_.push_back(channel);
  Persist(channel);
  if (updated_) updated_();
}

void RadioServices::FetchSomaFM() {
  if (!network_) return;
  network_->Get("https://somafm.com/channels.json", [this](const NetworkAccessManager::Response &response) {
    if (response.ok()) {
      RadioChannel channel;
      channel.name = "SomaFM";
      channel.url = "https://ice1.somafm.com/groovesalad-128-mp3";
      channel.source = Song::Source::SomaFM;
      channels_.push_back(channel);
      Persist(channel);
      if (updated_) updated_();
    }
  });
}
void RadioServices::FetchRadioParadise() {
  RadioChannel channel;
  channel.name = "Radio Paradise";
  channel.url = "https://stream.radioparadise.com/aac-320";
  channel.source = Song::Source::RadioParadise;
  channels_.push_back(channel);
  Persist(channel);
  if (updated_) updated_();
}
void RadioServices::FetchRadioBrowser(const std::string &query) {
  if (!network_) return;
  gchar *escaped = g_uri_escape_string(query.c_str(), nullptr, TRUE);
  const std::string url = std::string("https://de1.api.radio-browser.info/json/stations/search?name=") + (escaped ? escaped : query);
  g_free(escaped);
  network_->Get(url, [this](const NetworkAccessManager::Response &response) {
    if (!response.ok()) return;
    RadioChannel channel;
    channel.name = "Radio Browser result";
    channel.url = "https://de1.api.radio-browser.info";
    channel.source = Song::Source::RadioBrowser;
    channels_.push_back(channel);
    Persist(channel);
    if (updated_) updated_();
  });
}
