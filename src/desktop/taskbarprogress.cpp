#include "desktop/taskbarprogress.h"

TaskbarProgress::TaskbarProgress() = default;

TaskbarProgress::~TaskbarProgress() {
  if (visible_) {
    visible_ = false;
    Emit();
  }
  if (connection_) {
    g_object_unref(connection_);
    connection_ = nullptr;
  }
}

void TaskbarProgress::EnsureConnection() {
  if (connected_) {
    return;
  }
  connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  connected_ = true;
}

void TaskbarProgress::Emit() {
  EnsureConnection();
  if (!connection_) {
    return;
  }
  GVariantBuilder props;
  g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&props, "{sv}", "progress-visible", g_variant_new_boolean(visible_));
  g_variant_builder_add(&props, "{sv}", "progress", g_variant_new_double(visible_ ? fraction_ : 0.0));
  g_dbus_connection_emit_signal(connection_, nullptr, TaskbarProgressHelpers::ObjectPath().c_str(), TaskbarProgressHelpers::Interface(),
                                "Update", g_variant_new("(sa{sv})", TaskbarProgressHelpers::AppUri(), &props), nullptr);
}

void TaskbarProgress::Set(double fraction, bool visible) {
  const double clamped = std::clamp(fraction, 0.0, 1.0);
  if (clamped == fraction_ && visible == visible_) {
    return;
  }
  fraction_ = clamped;
  visible_ = visible;
  Emit();
}
