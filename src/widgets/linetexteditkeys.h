#ifndef STRAWBERRY_LINETEXTEDITKEYS_H
#define STRAWBERRY_LINETEXTEDITKEYS_H

#include "widgets/listboxkeyboard.h"

namespace LineTextEditKeys {

// Qt LineTextEdit::keyPressEvent ignores Enter/Return so the field stays one line
// and does not accept the Organize dialog.
inline bool IsEnter(unsigned keyval) {
  return keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter;
}

inline bool ShouldIgnore(unsigned keyval) { return IsEnter(keyval); }

}  // namespace LineTextEditKeys

#endif
