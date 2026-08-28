#ifndef STRAWBERRY_MACOSACCESSIBILITY_H
#define STRAWBERRY_MACOSACCESSIBILITY_H

namespace MacOsAccessibility {

#ifdef __APPLE__
inline bool IsMacOs() { return true; }
#else
inline bool IsMacOs() { return false; }
#endif

// Qt GlobalShortcutsSettingsPage shows widget_macos_access only on macOS when AX trust is missing.
inline bool ShouldShowAccessRow(bool is_macos, bool accessibility_enabled) { return is_macos && !accessibility_enabled; }

inline bool ShouldRefreshOnShow() { return true; }

// Qt IsAccessibilityEnabled passes kAXTrustedCheckOptionPrompt=@YES.
inline bool PromptWhenCheckingTrust() { return true; }

inline const char *GroupTitle() { return "Accessibility"; }

inline const char *Warning() {
  return "You need to launch System Settings and allow Orange to control your computer to use global shortcuts.";
}

inline const char *OpenButton() { return "Open..."; }

inline const char *SystemPreferencesBundle() { return "com.apple.systempreferences"; }

inline const char *SecurityPaneId() { return "com.apple.preference.security"; }

inline const char *AccessibilityAnchor() { return "Privacy_Accessibility"; }

inline const char *PreferencesUrl() { return "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"; }

}  // namespace MacOsAccessibility

#endif
