#ifndef STRAWBERRY_DISCORDSETTINGSRELOAD_H
#define STRAWBERRY_DISCORDSETTINGSRELOAD_H

namespace DiscordSettingsReload {

// Qt MainWindow::ReloadAllSettings always calls DiscordRichPresence::ReloadSettings.

inline bool ShouldReloadOnSettingsClose() { return true; }

inline bool ShouldDisconnectWhenDisabled(bool enabled) { return !enabled; }

}  // namespace DiscordSettingsReload

#endif
