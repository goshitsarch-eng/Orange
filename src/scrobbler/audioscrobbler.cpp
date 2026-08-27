#include "scrobbler/audioscrobbler.h"

#include "config.h"
#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#include "scrobbler/scrobblerlifecycle.h"
#include "scrobbler/scrobblersources.h"
#ifdef HAVE_SUBSONIC
#  include "scrobbler/subsonicscrobbler.h"
#endif

AudioScrobbler::AudioScrobbler(NetworkAccessManager *network) : network_(network) {
  services_.push_back(std::make_unique<LastFmScrobbler>(network));
  services_.push_back(std::make_unique<ListenBrainzScrobbler>(network));
#ifdef HAVE_SUBSONIC
  services_.push_back(std::make_unique<SubsonicScrobbler>(network));
#endif
  for (auto &service : services_) {
    service->Error.Connect([this](const std::string &message) { Error.Emit(message); });
  }
  ReloadSettings();
}

void AudioScrobbler::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  for (auto &service : services_) {
    service->set_enabled(settings.BoolValue(service->name(), false));
  }
}

bool AudioScrobbler::enabled() const {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  if (settings.BoolValue(ScrobblerSettings::kEnabled, ScrobblerSettings::kDefaultEnabled)) {
    return true;
  }
  for (const auto &service : services_) {
    if (service->enabled()) {
      return true;
    }
  }
  return false;
}

void AudioScrobbler::NowPlaying(const Song &song) {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  if (!ScrobblerSources::Allows(settings.Value(ScrobblerSettings::kSources), song.source())) {
    return;
  }
  for (auto &service : services_) {
    if (service->enabled()) {
      service->NowPlaying(song);
    }
  }
}

void AudioScrobbler::ClearPlaying() {
  for (auto &service : services_) {
    if (service->enabled()) {
      service->ClearPlaying();
    }
  }
}

void AudioScrobbler::Scrobble(const Song &song) {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  if (!ScrobblerSources::Allows(settings.Value(ScrobblerSettings::kSources), song.source())) {
    return;
  }
  for (auto &service : services_) {
    if (service->enabled()) {
      service->Scrobble(song);
    }
  }
}

void AudioScrobbler::Love(const Song &song) {
  for (auto &service : services_) {
    if (service->enabled()) {
      service->Love(song);
    }
  }
  TrackLoved.Emit(song);
}

void AudioScrobbler::ToggleScrobbling() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const bool enabled = settings.BoolValue(ScrobblerSettings::kEnabled, ScrobblerSettings::kDefaultEnabled);
  settings.SetBoolValue(ScrobblerSettings::kEnabled, !enabled);
  settings.Sync();
  EnabledChanged.Emit(this->enabled());
}

void AudioScrobbler::ToggleOffline() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const bool was_offline = settings.BoolValue(ScrobblerSettings::kOffline, ScrobblerSettings::kDefaultOffline);
  const bool now_offline = !was_offline;
  settings.SetBoolValue(ScrobblerSettings::kOffline, now_offline);
  settings.Sync();
  if (ScrobblerLifecycle::ShouldSubmitAfterOfflineToggle(was_offline, now_offline)) {
    Submit();
  }
}

void AudioScrobbler::WriteCache() {
  for (auto &service : services_) {
    service->WriteCache();
  }
}

void AudioScrobbler::Submit() {
  for (auto &service : services_) {
    if (service->enabled()) {
      service->Submit();
    }
  }
}

ScrobblerService *AudioScrobbler::ServiceByName(const std::string &name) const {
  for (const auto &service : services_) {
    if (service->name() == name) {
      return service.get();
    }
  }
  return nullptr;
}

std::vector<ScrobblerService *> AudioScrobbler::All() const {
  std::vector<ScrobblerService *> result;
  for (const auto &service : services_) {
    result.push_back(service.get());
  }
  return result;
}
