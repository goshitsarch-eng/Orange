#include "osd/osdbase.h"

#include "core/settings.h"
#include "osd/osdart.h"
#include "osd/osddbus.h"
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

bool OSDBase::SupportsNativeNotifications() const { return true; }

bool OSDBase::SupportsTrayPopups() const { return tray_icon_ != nullptr; }

bool OSDBase::SupportsOSDPretty() { return OSDPretty::Supported(); }

void OSDBase::ShowNative(const std::string &summary, const std::string &body, const std::string &icon,
                         const std::vector<unsigned char> &art) {
  if (dbus_) {
    dbus_->ShowMessage(summary, body, icon, OSDArt::EffectiveArt(show_art_, art));
  }
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
  if (!enabled_ || type_ == OSDSettings::Type::Disabled) {
    return;
  }
  switch (type_) {
    case OSDSettings::Type::Pretty:
      if (pretty_) {
        pretty_->ShowMessage(summary, body, show_art_ ? art : std::vector<unsigned char>());
      }
      break;
    case OSDSettings::Type::TrayPopup:
      if (tray_icon_) {
        tray_icon_->ShowPopup(summary, body, timeout_ms_, show_art_ ? art : std::vector<unsigned char>());
      } else {
        ShowNative(summary, body, icon, art);
      }
      break;
    case OSDSettings::Type::Native:
    case OSDSettings::Type::Disabled:
    default:
      ShowNative(summary, body, icon, art);
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

void OSDBase::ShowPreview(OSDSettings::Type type, const std::string &line1, const std::string &line2, const Song &) {
  if (type == OSDSettings::Type::Pretty && pretty_) {
    pretty_->ShowMessage(line1, line2);
    return;
  }
  if (type == OSDSettings::Type::TrayPopup && tray_icon_) {
    tray_icon_->ShowPopup(line1, line2, timeout_ms_);
    return;
  }
  ShowNative(line1, line2);
}
