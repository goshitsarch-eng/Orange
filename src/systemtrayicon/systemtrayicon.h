#ifndef STRAWBERRY_SYSTEMTRAYICON_H
#define STRAWBERRY_SYSTEMTRAYICON_H

#include "core/signal.h"
#include "core/song.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class SystemTrayIcon {
 public:
  SystemTrayIcon();
  ~SystemTrayIcon();

  void SetPlaying(bool playing, bool enable_play_pause = true);
  void SetPaused();
  void SetStopped();
  void SetProgress(int percentage);
  void SetNowPlaying(const Song &song);
  void ClearNowPlaying();
  void SetupStatusNotifier();
  void ReloadSettings();
  void SetVisible(bool visible);
  void ShowMenu(int x, int y);
  void ShowPopup(const std::string &summary, const std::string &message, int timeout_ms,
                 const std::vector<unsigned char> &art = {});
  void SetLoveVisible(bool visible);
  void SetLoveEnabled(bool enabled);
  void SetMuteEnabled(bool enabled);
  void SetMuteChecked(bool checked);
  bool love_visible() const { return love_visible_; }
  bool love_enabled() const { return love_enabled_; }
  bool mute_enabled() const { return mute_enabled_; }
  bool mute_checked() const { return mute_checked_; }
  bool play_pause_enabled() const { return play_pause_enabled_; }

  bool available() const { return available_; }
  bool visible() const { return visible_; }
  bool playing() const { return playing_; }
  bool paused() const { return paused_; }
  int progress() const { return progress_; }
  std::string OverlayIconName() const;
  int icon_pixmap_width() const { return icon_w_; }
  int icon_pixmap_height() const { return icon_h_; }
  size_t icon_pixmap_size() const { return icon_pixmap_.size(); }
  int last_menu_x() const { return last_menu_x_; }
  int last_menu_y() const { return last_menu_y_; }
  const std::vector<unsigned char> &popup_art() const { return popup_art_; }
  const std::string &tooltip() const { return tooltip_; }
  const std::string &popup_summary() const { return popup_summary_; }
  const std::string &popup_message() const { return popup_message_; }
  int popup_timeout_ms() const { return popup_timeout_ms_; }
  const std::string &menu_path() const { return menu_path_; }
  unsigned menu_revision() const { return menu_revision_; }

  static constexpr const char *kMenuObjectPath = "/MenuBar";
  static constexpr int kMenuPlayPause = 1;
  static constexpr int kMenuStop = 2;
  static constexpr int kMenuNext = 3;
  static constexpr int kMenuPrevious = 4;
  static constexpr int kMenuSeparator = 5;
  static constexpr int kMenuShowHide = 6;
  static constexpr int kMenuQuit = 7;
  static constexpr int kMenuMute = 8;
  static constexpr int kMenuStopAfter = 9;
  static constexpr int kMenuLove = 10;
  static const char *MenuLabel(int id, bool playing);
  static bool IsSeparatorId(int id) { return id == kMenuSeparator; }
  static std::vector<int> AllMenuIds() {
    return {kMenuPlayPause, kMenuStop, kMenuNext, kMenuPrevious, kMenuMute, kMenuStopAfter, kMenuLove, kMenuSeparator, kMenuShowHide,
            kMenuQuit};
  }
  static std::vector<int> RootMenuIds(bool show_love = true, bool show_mute = true);
  static bool ActivateMenuId(int id, Signal<> *play_pause, Signal<> *stop, Signal<> *next, Signal<> *previous, Signal<> *show_hide,
                             Signal<> *quit, Signal<> *mute = nullptr, Signal<> *stop_after = nullptr, Signal<> *love = nullptr);

  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> Next;
  Signal<> Previous;
  Signal<> ShowHide;
  Signal<> Quit;
  Signal<> Mute;
  Signal<> StopAfter;
  Signal<> Love;
  Signal<int> VolumeScroll;

  static void OnBusAcquired(GDBusConnection *connection, const gchar *name, gpointer data);
  static void OnNameLost(GDBusConnection *connection, const gchar *name, gpointer data);
  static void HandleMethod(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
                           const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
  static GVariant *HandleGetProperty(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                     const gchar *interface_name, const gchar *property_name, GError **error, gpointer data);
  static void HandleMenuMethod(GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
                               const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
  static GVariant *HandleMenuGetProperty(GDBusConnection *connection, const gchar *sender, const gchar *object_path,
                                         const gchar *interface_name, const gchar *property_name, GError **error, gpointer data);

 private:
  void UpdateTooltip();
  void RebuildIconPixmap();
  void RefreshPresentation();
  void EmitNewToolTip();
  void EmitNewStatus();
  void EmitNewOverlayIcon();
  void EmitNewIcon();
  void EmitLayoutUpdated();
  void PositionMenuWindow(GtkWidget *window, int x, int y);
  GVariant *MenuLayout(int parent_id) const;
  void RegisterMenu(GDBusConnection *connection);
  void TeardownStatusNotifier();

  guint owner_id_ = 0;
  guint registration_id_ = 0;
  guint menu_registration_id_ = 0;
  unsigned menu_revision_ = 1;
  std::string menu_path_ = "/NO_DBUSMENU";
  GDBusConnection *connection_ = nullptr;
  std::string service_name_;
  bool available_ = false;
  bool visible_ = false;
  bool playing_ = false;
  bool paused_ = false;
  int progress_ = 0;
  int icon_w_ = 0;
  int icon_h_ = 0;
  int last_menu_x_ = 0;
  int last_menu_y_ = 0;
  int popup_fade_gen_ = 0;
  std::vector<uint8_t> icon_pixmap_;
  std::vector<unsigned char> popup_art_;
  Song song_;
  std::string tooltip_ = "Strawberry";
  std::string popup_summary_;
  std::string popup_message_;
  int popup_timeout_ms_ = 0;
  bool love_visible_ = true;
  bool love_enabled_ = true;
  bool mute_enabled_ = true;
  bool mute_checked_ = false;
  bool play_pause_enabled_ = true;
  GtkWidget *popup_window_ = nullptr;
  GtkWidget *popup_title_ = nullptr;
  GtkWidget *popup_body_ = nullptr;
  GtkWidget *popup_image_ = nullptr;
  guint popup_timeout_id_ = 0;
};

#endif
