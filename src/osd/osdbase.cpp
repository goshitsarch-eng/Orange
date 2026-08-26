#include "osd/osdbase.h"

#include "core/settings.h"
#include "osd/osddbus.h"
#include "osd/osdpretty.h"

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

bool OSDBase::SupportsOSDPretty() { return true; }

void OSDBase::ShowNative(const std::string &summary, const std::string &body, const std::string &icon) {
  if (dbus_) {
    dbus_->ShowMessage(summary, body, icon);
  }
}

void OSDBase::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon) {
  if (!enabled_) {
    return;
  }
  ShowNative(summary, body, icon);
}

void OSDBase::SongChanged(const Song &song, const std::vector<unsigned char> &art) {
  if (!enabled_ || type_ == OSDSettings::Type::Disabled) {
    return;
  }
  const std::string body = song.EffectiveAlbumartist() + (song.album().empty() ? "" : "\n" + song.album());
  if (type_ == OSDSettings::Type::Pretty && pretty_) {
    pretty_->ShowMessage(song.PrettyTitle(), body, show_art_ ? art : std::vector<unsigned char>());
  }
  if (type_ == OSDSettings::Type::Native || type_ == OSDSettings::Type::TrayPopup) {
    ShowNative(song.PrettyTitle(), body);
  }
}

void OSDBase::Paused() { ShowMessage("Paused", {}); }

void OSDBase::Resumed() { ShowMessage("Playing", {}); }

void OSDBase::Stopped() { ShowMessage("Stopped", {}); }

void OSDBase::VolumeChanged(unsigned value) { ShowMessage("Volume", std::to_string(value) + "%"); }

void OSDBase::ShowPreview(OSDSettings::Type type, const std::string &line1, const std::string &line2, const Song &) {
  if (type == OSDSettings::Type::Pretty && pretty_) {
    pretty_->ShowMessage(line1, line2);
    return;
  }
  ShowNative(line1, line2);
}
