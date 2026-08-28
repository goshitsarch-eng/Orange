#ifndef STRAWBERRY_OSDNATIVE_H
#define STRAWBERRY_OSDNATIVE_H

namespace OSDNative {

inline bool IsWindows() {
#ifdef _WIN32
  return true;
#else
  return false;
#endif
}

inline bool IsMacOs() {
#ifdef __APPLE__
  return true;
#else
  return false;
#endif
}

// Qt OSDBase::SupportsNativeNotifications is tray-backed on Windows and false elsewhere.
// Linux Qt uses the OSDDbus subclass; GTK ShowNative is D-Bus on Linux.
inline bool BaseSupportsNativeNotifications(bool tray_available) { return IsWindows() && tray_available; }

inline bool UsesDbusNative() { return !IsWindows() && !IsMacOs(); }

inline bool UsesNotificationCenter() { return IsMacOs(); }

inline bool SupportsNativeNotifications(bool tray_available) {
  if (IsWindows()) {
    return tray_available;
  }
  return true;
}

// Qt OSDMac::SupportsTrayPopups is false. Other platforms use the tray icon.
inline bool SupportsTrayPopups(bool tray_available) {
  if (IsMacOs()) {
    return false;
  }
  return tray_available;
}

// Qt OSDBase::ShowMessage: Windows Native falls through to TrayPopup.
inline bool NativeFallsThroughToTray() { return IsWindows(); }

// Qt OSDBase::ShowMessage: macOS TrayPopup falls through to Pretty.
inline bool TrayFallsThroughToPretty() { return IsMacOs(); }

}  // namespace OSDNative

#endif
