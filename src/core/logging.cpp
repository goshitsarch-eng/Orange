#include "core/logging.h"

#include "config.h"

#include <cstdarg>

namespace logging {

namespace {
bool g_debug_enabled =
#ifdef ENABLE_DEBUG_OUTPUT
    true;
#else
    false;
#endif
}  // namespace

void Init() {
  g_log_set_handler(nullptr, static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
                    [](const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer) {
                      g_log_default_handler(log_domain, log_level, message, nullptr);
                    },
                    nullptr);
}

void SetDebugEnabled(bool enabled) { g_debug_enabled = enabled; }

void SetLevels(const std::string &levels) {
  if (levels.empty()) {
    return;
  }
  if (levels.find("*:4") != std::string::npos || levels.find(":4") != std::string::npos) {
    SetDebugEnabled(true);
    return;
  }
  if (levels.find("*:1") != std::string::npos || levels == "1") {
    SetDebugEnabled(false);
  }
}

void Log(Level level, const char *domain, const char *format, ...) {
  if (level == Level::Debug && !g_debug_enabled) {
    return;
  }

  GLogLevelFlags glib_level = G_LOG_LEVEL_MESSAGE;
  switch (level) {
    case Level::Debug:
      glib_level = G_LOG_LEVEL_DEBUG;
      break;
    case Level::Info:
      glib_level = G_LOG_LEVEL_INFO;
      break;
    case Level::Warning:
      glib_level = G_LOG_LEVEL_WARNING;
      break;
    case Level::Error:
      glib_level = G_LOG_LEVEL_CRITICAL;
      break;
  }

  va_list args;
  va_start(args, format);
  g_logv(domain, glib_level, format, args);
  va_end(args);
}

}  // namespace logging
