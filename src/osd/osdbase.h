#ifndef STRAWBERRY_OSDBASE_H
#define STRAWBERRY_OSDBASE_H

#include "constants/notificationssettings.h"
#include "core/song.h"

#include <string>
#include <vector>

class OSDPretty;
class OSDDbus;
class SystemTrayIcon;

class OSDBase {
 public:
  explicit OSDBase(SystemTrayIcon *tray_icon = nullptr);
  virtual ~OSDBase();

  int timeout_msec() const { return timeout_ms_; }
  void ReloadPrettyOSDSettings();
  void ReloadSettings();

  OSDSettings::Type GetSupportedType() const;
  bool IsTypeSupported(OSDSettings::Type type) const;
  virtual bool SupportsNativeNotifications() const;
  virtual bool SupportsTrayPopups() const;
  static bool SupportsOSDPretty();

  void SongChanged(const Song &song, const std::vector<unsigned char> &art = {});
  void Paused();
  void Resumed();
  void Stopped();
  void VolumeChanged(unsigned value);
  void ShowPreview(OSDSettings::Type type, const std::string &line1, const std::string &line2, const Song &song);
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");
  bool enabled() const { return enabled_; }

 protected:
  virtual void ShowNative(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");

  bool enabled_ = true;
  bool show_art_ = true;
  OSDSettings::Type type_ = OSDSettings::Type::Native;
  int timeout_ms_ = 4000;
  SystemTrayIcon *tray_icon_ = nullptr;
  OSDPretty *pretty_ = nullptr;
  OSDDbus *dbus_ = nullptr;
};

#endif
