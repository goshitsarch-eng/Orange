#ifndef STRAWBERRY_TRAYSETTINGSRELOAD_H
#define STRAWBERRY_TRAYSETTINGSRELOAD_H

namespace TraySettingsReload {

// Qt MainWindow::ReloadSettings setVisible(showtrayicon) and show() if tray is off while hidden.

inline bool ShouldReloadOnSettingsClose() { return true; }

inline bool ShowTray(bool stored) { return stored; }

inline bool IsRegistered(unsigned owner_id) { return owner_id != 0; }

inline bool ShouldRegister(bool show_tray, bool registered) { return show_tray && !registered; }

inline bool ShouldUnregister(bool show_tray, bool registered) { return !show_tray && registered; }

inline bool ShouldRefreshProgress() { return true; }

inline bool ShouldPresentWindowAfterDisable(bool show_tray, bool window_visible) { return !show_tray && !window_visible; }

}  // namespace TraySettingsReload

#endif
