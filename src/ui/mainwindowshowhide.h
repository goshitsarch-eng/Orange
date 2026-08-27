#ifndef STRAWBERRY_MAINWINDOWSHOWHIDE_H
#define STRAWBERRY_MAINWINDOWSHOWHIDE_H

namespace MainWindowShowHide {

// Qt MainWindow::ToggleShowHide / SetHiddenInTray / closeEvent keep_running + tray.

enum class Action { Present, HideToTray, Minimize, Exit };

inline bool EffectiveKeepRunning(bool tray_available, bool tray_visible, bool keep_running) {
  return tray_available && tray_visible && keep_running;
}

inline bool ShouldHideInsteadOfExit(bool keep_running_effective) { return keep_running_effective; }

inline Action CloseAction(bool keep_running_effective) { return keep_running_effective ? Action::HideToTray : Action::Exit; }

inline Action HideAction(bool keep_running_effective) { return keep_running_effective ? Action::HideToTray : Action::Minimize; }

inline Action ShortcutAction(bool window_visible, bool window_active, bool keep_running_effective) {
  if (!window_visible) {
    return Action::Present;
  }
  if (window_active) {
    return HideAction(keep_running_effective);
  }
  return Action::Present;
}

}  // namespace MainWindowShowHide

#endif
