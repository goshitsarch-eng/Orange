#ifndef STRAWBERRY_LOGGING_H
#define STRAWBERRY_LOGGING_H

#include <glib.h>
#include <string>

namespace logging {

enum class Level { Debug, Info, Warning, Error };

void Init();
void SetDebugEnabled(bool enabled);
void Log(Level level, const char *domain, const char *format, ...) G_GNUC_PRINTF(3, 4);

}  // namespace logging

#define qLog(level) logging::Log

#define LogDebug(...) logging::Log(logging::Level::Debug, G_LOG_DOMAIN, __VA_ARGS__)
#define LogInfo(...) logging::Log(logging::Level::Info, G_LOG_DOMAIN, __VA_ARGS__)
#define LogWarning(...) logging::Log(logging::Level::Warning, G_LOG_DOMAIN, __VA_ARGS__)
#define LogError(...) logging::Log(logging::Level::Error, G_LOG_DOMAIN, __VA_ARGS__)

#endif
