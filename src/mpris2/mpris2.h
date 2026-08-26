#ifndef STRAWBERRY_MPRIS2_H
#define STRAWBERRY_MPRIS2_H

#include <gio/gio.h>

#include <cstdint>
#include <string>

class Application;

class Mpris2 {
 public:
  explicit Mpris2(Application *app);
  ~Mpris2();
  void EmitSeeked(int64_t position_us);
  void EmitPlaybackStatus();
  void EmitMetadata();
  void EmitVolume();
  Application *app() const { return app_; }

 private:
  static void OnBusAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
  void EmitPropertiesChanged(const char *interface_name, const char *property, GVariant *value);

  Application *app_;
  guint owner_id_ = 0;
  GDBusConnection *connection_ = nullptr;
};

#endif
