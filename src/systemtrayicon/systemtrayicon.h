#ifndef STRAWBERRY_SYSTEMTRAYICON_H
#define STRAWBERRY_SYSTEMTRAYICON_H

#include "core/signal.h"
#include "core/song.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

#include <string>

class SystemTrayIcon {
 public:
  SystemTrayIcon();
  ~SystemTrayIcon();

  void SetPlaying(bool playing);
  void SetProgress(int percentage);
  void SetNowPlaying(const Song &song);
  void ClearNowPlaying();
  void SetupStatusNotifier();
  void SetVisible(bool visible);
  void ShowMenu(int x, int y);

  bool available() const { return available_; }
  bool visible() const { return visible_; }
  bool playing() const { return playing_; }
  int progress() const { return progress_; }
  const std::string &tooltip() const { return tooltip_; }
  const std::string &menu_path() const { return menu_path_; }
  unsigned menu_revision() const { return menu_revision_; }

  static constexpr const char *kMenuObjectPath = "/MenuBar";
  static const char *MenuLabel(int id, bool playing);
  static bool ActivateMenuId(int id, Signal<> *play_pause, Signal<> *stop, Signal<> *next, Signal<> *previous, Signal<> *show_hide,
                             Signal<> *quit);

  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> Next;
  Signal<> Previous;
  Signal<> ShowHide;
  Signal<> Quit;
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
  void EmitNewToolTip();
  void EmitNewStatus();
  void EmitLayoutUpdated();
  GVariant *MenuLayout(int parent_id) const;
  void RegisterMenu(GDBusConnection *connection);

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
  int progress_ = 0;
  Song song_;
  std::string tooltip_ = "Strawberry";
};

#endif
