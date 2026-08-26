#ifndef STRAWBERRY_MPRIS2_H
#define STRAWBERRY_MPRIS2_H
#include <gio/gio.h>
class Application;
class Mpris2 {
 public:
  explicit Mpris2(Application *app);
  ~Mpris2();
  void EmitSeeked(int64_t position_us);
  void EmitPlaybackStatus();
 private:
  static void OnBusAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);
  Application *app_;
  guint owner_id_ = 0;
  GDBusConnection *connection_ = nullptr;
};
#endif
