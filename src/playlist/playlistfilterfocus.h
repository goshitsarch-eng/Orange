#ifndef STRAWBERRY_PLAYLISTFILTERFOCUS_H
#define STRAWBERRY_PLAYLISTFILTERFOCUS_H

#include "playlist/playlistkeyboard.h"
#include "widgets/listboxkeyboard.h"

#include <string>

namespace PlaylistFilterFocus {

enum class Effect { None, Append, Clear, FocusOnly };

// Qt PlaylistView::keyPressEvent: Key_Exclam..Key_Z. GDK letter keyvals also include a–z.
inline bool IsFilterCharKey(unsigned keyval) {
  return (keyval >= 0x021 && keyval <= 0x05a) || (keyval >= 0x061 && keyval <= 0x07a);
}

// Qt requires NoModifier (so Ctrl+C still copies). Space is PlayPause, not a filter character.
inline bool ShouldRouteToFilter(unsigned keyval, unsigned modifiers, bool printable_nonspace,
                                unsigned control_mask = PlaylistKeyboard::kControlMask) {
  if ((modifiers & control_mask) != 0) {
    return false;
  }
  if (keyval == PlaylistKeyboard::kSpace || keyval == PlaylistKeyboard::kKPSpace) {
    return false;
  }
  if (keyval == ListBoxKeyboard::kBackSpace || keyval == ListBoxKeyboard::kEscape) {
    return true;
  }
  return printable_nonspace || IsFilterCharKey(keyval);
}

inline Effect KeyEffect(unsigned keyval) {
  if (keyval == ListBoxKeyboard::kEscape) {
    return Effect::Clear;
  }
  if (keyval == ListBoxKeyboard::kBackSpace) {
    return Effect::FocusOnly;
  }
  if (IsFilterCharKey(keyval)) {
    return Effect::Append;
  }
  return Effect::None;
}

inline std::string AppendUtf8(const std::string &current, const char *utf8) {
  if (!utf8 || utf8[0] == '\0') {
    return current;
  }
  return current + utf8;
}

// Qt FocusOnFilter: Escape clears; Backspace only focuses; other keys append event->text().
inline std::string Apply(const std::string &current, Effect effect, const char *utf8) {
  if (effect == Effect::Clear) {
    return {};
  }
  if (effect == Effect::Append) {
    return AppendUtf8(current, utf8);
  }
  return current;
}

// Qt accepts the key and never typeahead-jumps playlist rows (toolbar hidden or not).
inline bool ShouldTypeahead(bool routed_to_filter) { return !routed_to_filter; }

}  // namespace PlaylistFilterFocus

#endif
