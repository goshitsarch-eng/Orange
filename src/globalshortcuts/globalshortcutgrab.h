#ifndef STRAWBERRY_GLOBALSHORTCUTGRAB_H
#define STRAWBERRY_GLOBALSHORTCUTGRAB_H

#include <gdk/gdk.h>

#include <string>

namespace GlobalShortcutGrab {

inline const char *WindowTitle() { return "Press a key"; }
inline const char *WaitingLabel() { return "Waiting…"; }

inline std::string Prompt(const std::string &action) {
  if (action.empty()) {
    return "Press a key combination.";
  }
  return "Press a key combination to use for " + action + "...";
}

inline bool IsModifier(guint keyval) {
  switch (keyval) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_ISO_Level3_Shift:
      return true;
    default:
      return false;
  }
}

inline bool ShouldAccept(guint keyval) { return !IsModifier(keyval); }

inline std::string PreviewMarkup(const std::string &accel) {
  if (accel.empty()) {
    return {};
  }
  return "<b>" + accel + "</b>";
}

inline bool RejectClears(const std::string &preview) { return preview.empty(); }

// Qt GlobalShortcutGrabber::Rejected: Cancel only closes when the preview is empty.
inline bool ShouldDismissOnCancel(const std::string &accel) { return RejectClears(PreviewMarkup(accel)); }

}  // namespace GlobalShortcutGrab

#endif  // STRAWBERRY_GLOBALSHORTCUTGRAB_H
