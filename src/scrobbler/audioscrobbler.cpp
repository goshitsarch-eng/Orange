#include "scrobbler/audioscrobbler.h"

#include "config.h"
#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "scrobbler/lastfmscrobbler.h"
#include "scrobbler/listenbrainzscrobbler.h"
#ifdef HAVE_SUBSONIC
#  include "scrobbler/subsonicscrobbler.h"
#endif

AudioScrobbler::AudioScrobbler(NetworkAccessManager *network) : network_(network) {
  services_.push_back(std::make_unique<LastFmScrobbler>(network));
  services_.push_back(std::make_unique<ListenBrainzScrobbler>(network));
#ifdef HAVE_SUBSONIC
  services_.push_back(std::make_unique<SubsonicScrobbler>(network));
#endif
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
  for (auto &service : services_) {
    if (service->enabled()) {
      service->NowPlaying(song);
    }
  }
}

void AudioScrobbler::Scrobble(const Song &song) {
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
}

void AudioScrobbler::ToggleScrobbling() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const bool enabled = settings.BoolValue(ScrobblerSettings::kEnabled, ScrobblerSettings::kDefaultEnabled);
  settings.SetBoolValue(ScrobblerSettings::kEnabled, !enabled);
  settings.Sync();
}

std::vector<ScrobblerService *> AudioScrobbler::All() const {
  std::vector<ScrobblerService *> result;
  for (const auto &service : services_) {
    result.push_back(service.get());
  }
  return result;
}
