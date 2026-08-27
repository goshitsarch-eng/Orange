#include "engine/gststartup.h"

#include "core/logging.h"

#include <gst/gst.h>

namespace GstStartup {

void SetEnvironment() {
  if (!g_getenv("GST_PLUGIN_SYSTEM_PATH") && !g_getenv("GST_PLUGIN_PATH")) {
    g_setenv("GST_REGISTRY_UPDATE", "no", FALSE);
  }
}

void Initialize() {
  SetEnvironment();
  GError *error = nullptr;
  if (!gst_is_initialized() && !gst_init_check(nullptr, nullptr, &error)) {
    if (error) {
      LogError("GStreamer init failed: %s", error->message);
      g_error_free(error);
    }
  }
}

}  // namespace GstStartup
