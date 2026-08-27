#ifndef STRAWBERRY_OSDBASE_H
#define STRAWBERRY_OSDBASE_H

#include "constants/notificationssettings.h"
#include "core/song.h"
#include "playlist/playlistsequence.h"

#include <string>
#include <vector>

class OSDPretty;
class OSDDbus;
class SystemTrayIcon;

class OSDBase {
 public:
  explicit OSDBase(SystemTrayIcon *tray_icon = nullptr);
  virtual ~OSDBase();

  void set_tray_icon(SystemTrayIcon *tray_icon) { tray_icon_ = tray_icon; }
  int timeout_msec() const { return timeout_ms_; }
  void ReloadPrettyOSDSettings();
  void ReloadSettings();

  OSDSettings::Type GetSupportedType() const;
  bool IsTypeSupported(OSDSettings::Type type) const;
  virtual bool SupportsNativeNotifications() const;
  virtual bool SupportsTrayPopups() const;
  static bool SupportsOSDPretty();

  void SongChanged(const Song &song, const std::vector<unsigned char> &art = {});
  void AlbumCoverLoaded(const Song &song, const std::vector<unsigned char> &art);
  void Paused();
  void Resumed();
  void Stopped();
  void StopAfterToggle(bool stop);
  void PlaylistFinished();
  void VolumeChanged(unsigned value);
  void PlayModeChanged(const std::string &mode);
  void RepeatModeChanged(PlaylistSequence::RepeatMode mode);
  void ShuffleModeChanged(PlaylistSequence::ShuffleMode mode);
  void SetPrettyOSDToggleMode(bool toggle);
  void ShowPreview(OSDSettings::Type type, const std::string &line1, const std::string &line2, const Song &song);
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon, const std::vector<unsigned char> &art);
  bool enabled() const { return enabled_; }
  OSDSettings::Type type() const { return type_; }

 protected:
  virtual void ShowNative(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic",
                          const std::vector<unsigned char> &art = {});
  std::string PlayingSummary(const Song &song) const;
  std::string PlayingBody(const Song &song) const;

  bool enabled_ = true;
  bool show_art_ = true;
  bool show_on_volume_change_ = false;
  bool show_on_play_mode_change_ = true;
  bool show_on_pause_ = true;
  bool show_on_resume_ = false;
  bool use_custom_text_ = false;
  bool playing_ = false;
  bool ignore_next_stopped_ = false;
  OSDSettings::Type type_ = OSDSettings::Type::Native;
  int timeout_ms_ = 4000;
  std::string custom_text1_;
  std::string custom_text2_;
  Song last_song_;
  std::vector<unsigned char> last_art_;
  SystemTrayIcon *tray_icon_ = nullptr;
  OSDPretty *pretty_ = nullptr;
  OSDDbus *dbus_ = nullptr;
};

#endif
