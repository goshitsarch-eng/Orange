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

 private:
  void UpdateTooltip();
  void EmitNewToolTip();
  void EmitNewStatus();

  guint owner_id_ = 0;
  guint registration_id_ = 0;
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
