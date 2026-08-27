#include "osd/osdbase.h"

#include "core/settings.h"
#include "osd/osdforce.h"
#include "osd/osdart.h"
#include "osd/osdartrefresh.h"
#include "osd/osddbus.h"
#include "osd/osdmac.h"
#include "osd/osdnative.h"
#include "osd/osdpretty.h"
#include "systemtrayicon/systemtrayicon.h"
#include "translations/translations.h"
#include "utilities/strutils.h"

OSDBase::OSDBase(SystemTrayIcon *tray_icon) : tray_icon_(tray_icon) {
  pretty_ = new OSDPretty(OSDPretty::Mode::Popup);
  dbus_ = new OSDDbus();
}

OSDBase::~OSDBase() {
  delete pretty_;
  delete dbus_;
}

void OSDBase::ReloadPrettyOSDSettings() {
  if (pretty_) {
    pretty_->ReloadSettings();
    pretty_->set_popup_duration(timeout_ms_);
  }
}

void OSDBase::ReloadSettings() {
  Settings s;
  s.BeginGroup(OSDSettings::kSettingsGroup);
  if (s.Contains(OSDSettings::kType) || s.Contains("Behaviour")) {
    const int type = s.IntValue(s.Contains(OSDSettings::kType) ? OSDSettings::kType : "Behaviour", 1);
    enabled_ = type != 0;
    type_ = static_cast<OSDSettings::Type>(type);
  } else {
    enabled_ = s.BoolValue("enabled", true);
    const std::string type = s.Value("type", "native");
    type_ = type == "pretty" ? OSDSettings::Type::Pretty : (type == "tray" ? OSDSettings::Type::TrayPopup : OSDSettings::Type::Native);
  }
  show_art_ = s.Contains(OSDSettings::kShowArt) ? s.BoolValue(OSDSettings::kShowArt, true) : s.BoolValue("showart", true);
  timeout_ms_ = s.Contains(OSDSettings::kTimeout) ? s.IntValue(OSDSettings::kTimeout, 5000) : s.IntValue("timeout", 4000);
  show_on_volume_change_ = s.BoolValue(OSDSettings::kShowOnVolumeChange, OSDSettings::kDefaultShowOnVolumeChange);
  show_on_play_mode_change_ = s.BoolValue(OSDSettings::kShowOnPlayModeChange, OSDSettings::kDefaultShowOnPlayModeChange);
  show_on_pause_ = s.BoolValue(OSDSettings::kShowOnPausePlayback, OSDSettings::kDefaultShowOnPausePlayback);
  show_on_resume_ = s.BoolValue(OSDSettings::kShowOnResumePlayback, OSDSettings::kDefaultShowOnResumePlayback);
  use_custom_text_ = s.BoolValue(OSDSettings::kCustomTextEnabled, OSDSettings::kDefaultCustomTextEnabled);
  custom_text1_ = s.Value(OSDSettings::kCustomText1);
  custom_text2_ = s.Value(OSDSettings::kCustomText2);
  ReloadPrettyOSDSettings();
}

OSDSettings::Type OSDBase::GetSupportedType() const {
  if (SupportsOSDPretty()) {
    return OSDSettings::Type::Pretty;
  }
  if (SupportsNativeNotifications()) {
    return OSDSettings::Type::Native;
  }
  return OSDSettings::Type::TrayPopup;
}

bool OSDBase::IsTypeSupported(OSDSettings::Type type) const {
  switch (type) {
    case OSDSettings::Type::Disabled:
      return true;
    case OSDSettings::Type::Native:
      return SupportsNativeNotifications();
    case OSDSettings::Type::TrayPopup:
      return SupportsTrayPopups();
    case OSDSettings::Type::Pretty:
      return SupportsOSDPretty();
  }
  return false;
}

bool OSDBase::SupportsNativeNotifications() const { return OSDNative::SupportsNativeNotifications(tray_icon_ != nullptr); }

bool OSDBase::SupportsTrayPopups() const { return OSDNative::SupportsTrayPopups(tray_icon_ != nullptr); }

bool OSDBase::SupportsOSDPretty() { return OSDPretty::Supported(); }

void OSDBase::ShowNative(const std::string &summary, const std::string &body, const std::string &icon,
                         const std::vector<unsigned char> &art) {
#ifdef __APPLE__
  OSDMac::ShowMessageNative(summary, body, icon, OSDArt::EffectiveArt(show_art_, art));
#else
  if (dbus_) {
    dbus_->ShowMessage(summary, body, icon, OSDArt::EffectiveArt(show_art_, art));
  }
#endif
}

std::string OSDBase::PlayingSummary(const Song &song) const {
  if (use_custom_text_ && !custom_text1_.empty()) {
    return StrUtils::ReplaceMessage(custom_text1_, song);
  }
  if (!song.artist().empty()) {
    return song.artist() + " - " + song.PrettyTitle();
  }
  return song.PrettyTitle();
}

std::string OSDBase::PlayingBody(const Song &song) const {
  if (use_custom_text_ && !custom_text2_.empty()) {
    return StrUtils::ReplaceMessage(custom_text2_, song);
  }
  std::vector<std::string> parts;
  if (!song.album().empty()) {
    parts.push_back(song.album());
  }
  if (song.disc() > 0) {
    parts.push_back(StrUtils::Replace(Translations::Tr("disc %1"), "%1", std::to_string(song.disc())));
  }
  if (song.track() > 0) {
    parts.push_back(StrUtils::Replace(Translations::Tr("track %1"), "%1", std::to_string(song.track())));
  }
  return StrUtils::Join(parts, ", ");
}

void OSDBase::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon) {
  ShowMessage(summary, body, icon, {});
}

void OSDBase::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon,
                          const std::vector<unsigned char> &art) {
  if (pretty_ && pretty_->toggle_mode()) {
    pretty_->ShowMessage(summary, body, show_art_ ? art : std::vector<unsigned char>());
    return;
  }
  OSDSettings::Type type = type_;
  if (type == OSDSettings::Type::Disabled) {
    if (!OSDForce::ConsumeForce(&force_show_next_)) {
      return;
    }
    type = OSDSettings::Type::Pretty;
  } else if (!enabled_) {
    return;
  }
  switch (type) {
    case OSDSettings::Type::Native:
      if (!OSDNative::NativeFallsThroughToTray()) {
        ShowNative(summary, body, icon, art);
        break;
      }
      [[fallthrough]];
    case OSDSettings::Type::TrayPopup:
      if (!OSDNative::TrayFallsThroughToPretty()) {
        if (tray_icon_) {
          tray_icon_->ShowPopup(summary, body, timeout_ms_, show_art_ ? art : std::vector<unsigned char>());
        } else {
          ShowNative(summary, body, icon, art);
        }
        break;
      }
      [[fallthrough]];
    case OSDSettings::Type::Pretty:
    case OSDSettings::Type::Disabled:
    default:
      if (pretty_) {
        pretty_->ShowMessage(summary, body, show_art_ ? art : std::vector<unsigned char>());
      } else if (tray_icon_) {
        tray_icon_->ShowPopup(summary, body, timeout_ms_, show_art_ ? art : std::vector<unsigned char>());
      } else {
        ShowNative(summary, body, icon, art);
      }
      break;
  }
}

void OSDBase::SongChanged(const Song &song, const std::vector<unsigned char> &art) {
  playing_ = true;
  last_song_ = song;
  last_art_ = art;
  if (!enabled_ || type_ == OSDSettings::Type::Disabled) {
    return;
  }
  ShowMessage(PlayingSummary(song), PlayingBody(song), "notification-audio-play", art);
}

void OSDBase::ReshowCurrentSong(const Song &song, const std::vector<unsigned char> &art) {
  if (song.is_valid()) {
    last_song_ = song;
    last_art_ = art;
    playing_ = true;
  }
  force_show_next_ = true;
  ShowMessage(PlayingSummary(last_song_), PlayingBody(last_song_), "notification-audio-play", last_art_);
}

void OSDBase::AlbumCoverLoaded(const Song &song, const std::vector<unsigned char> &art) {
  if (!playing_ || !OsdArtRefresh::MatchesPlaying(last_song_, song)) {
    return;
  }
  const bool previous_empty = last_art_.empty();
  if (!OsdArtRefresh::ShouldRefresh(show_art_, previous_empty, !art.empty())) {
    last_art_ = art;
    return;
  }
  last_art_ = art;
  if (!enabled_ || type_ == OSDSettings::Type::Disabled) {
    return;
  }
  ShowMessage(PlayingSummary(last_song_), PlayingBody(last_song_), "notification-audio-play", art);
}

void OSDBase::Paused() {
  if (!show_on_pause_) {
    return;
  }
  ShowMessage(PlayingSummary(last_song_), Translations::Tr("Paused"), {}, last_art_);
}

void OSDBase::Resumed() {
  if (!show_on_resume_) {
    return;
  }
  ShowMessage(PlayingSummary(last_song_), PlayingBody(last_song_), "notification-audio-play", last_art_);
}

void OSDBase::Stopped() {
  if (!playing_) {
    return;
  }
  playing_ = false;
  if (ignore_next_stopped_) {
    ignore_next_stopped_ = false;
    last_song_ = Song();
    last_art_.clear();
    return;
  }
  ShowMessage(PlayingSummary(last_song_), Translations::Tr("Stopped"), {}, last_art_);
  last_song_ = Song();
  last_art_.clear();
}

void OSDBase::StopAfterToggle(bool stop) {
  ShowMessage("Strawberry", StrUtils::Replace(Translations::Tr("Stop playing after track: %1"), "%1",
                                              stop ? Translations::Tr("On") : Translations::Tr("Off")));
}

void OSDBase::PlaylistFinished() {
  ignore_next_stopped_ = true;
  ShowMessage("Strawberry", Translations::Tr("Playlist finished"));
}

void OSDBase::VolumeChanged(unsigned value) {
  if (!show_on_volume_change_) {
    return;
  }
  ShowMessage("Strawberry", StrUtils::Replace(Translations::Tr("Volume %1%"), "%1", std::to_string(value)));
}

void OSDBase::PlayModeChanged(const std::string &mode) {
  if (!show_on_play_mode_change_ || mode.empty()) {
    return;
  }
  ShowMessage("Strawberry", mode);
}

void OSDBase::RepeatModeChanged(PlaylistSequence::RepeatMode mode) { PlayModeChanged(PlaylistSequence::RepeatLabel(mode)); }

void OSDBase::ShuffleModeChanged(PlaylistSequence::ShuffleMode mode) { PlayModeChanged(PlaylistSequence::ShuffleLabel(mode)); }

void OSDBase::SetPrettyOSDToggleMode(bool toggle) {
  if (pretty_) {
    pretty_->set_toggle_mode(toggle);
  }
}

void OSDBase::ShowPreview(OSDSettings::Type type, const std::string &line1, const std::string &line2, const Song &song) {
  type_ = type;
  enabled_ = type != OSDSettings::Type::Disabled;
  const std::string summary = StrUtils::ReplaceMessage(line1, song);
  const std::string body = StrUtils::ReplaceMessage(line2, song);
  if (type == OSDSettings::Type::Pretty && pretty_) {
    pretty_->ShowMessage(summary, body);
    return;
  }
  if (type == OSDSettings::Type::TrayPopup && tray_icon_) {
    tray_icon_->ShowPopup(summary, body, timeout_ms_);
    return;
  }
  ShowNative(summary, body);
}
