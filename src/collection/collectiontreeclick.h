#ifndef COLLECTIONTREECLICK_H
#define COLLECTIONTREECLICK_H

#include "config.h"

#include <gdk/gdk.h>

namespace CollectionTreeClick {

inline constexpr guint kPrimaryButton = 1;
inline constexpr guint kMiddleButton = 2;
inline constexpr guint kSecondaryButton = 3;

enum class Action {
  None,
  ToggleExpand,
  Enqueue
};

inline bool HasModifier(const GdkModifierType state) {
  return (state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK | GDK_SUPER_MASK)) != 0;
}

inline Action FromPress(const guint button, const gint n_press, const GdkModifierType state) {
  if (button == kMiddleButton && n_press >= 1) {
    return Action::Enqueue;
  }
  if (button != kPrimaryButton || HasModifier(state) || n_press != 1) {
    return Action::None;
  }
  return Action::ToggleExpand;
}

inline bool ShouldToggleFromRowClick(const bool on_expand_control, const bool expandable) {
  return !on_expand_control && expandable;
}

inline bool SelectRowBeforeEnqueue(const bool row_already_selected) { return !row_already_selected; }

}  // namespace CollectionTreeClick

#endif  // COLLECTIONTREECLICK_H
