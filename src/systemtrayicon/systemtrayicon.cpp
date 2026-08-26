#include "systemtrayicon/systemtrayicon.h"
#include "core/logging.h"
SystemTrayIcon::SystemTrayIcon() { available_ = true; }
SystemTrayIcon::~SystemTrayIcon() = default;
void SystemTrayIcon::SetPlaying(bool playing) { playing_ = playing; }
void SystemTrayIcon::SetProgress(int) {}
void SystemTrayIcon::SetNowPlaying(const Song &) {}
void SystemTrayIcon::SetupStatusNotifier() { available_ = true; }
