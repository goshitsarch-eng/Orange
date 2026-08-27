#ifndef STRAWBERRY_COLLECTIONFILTERFOCUS_H
#define STRAWBERRY_COLLECTIONFILTERFOCUS_H

#include "widgets/listboxkeyboard.h"

#include <string>

namespace CollectionFilterFocus {

enum class Effect { None, Clear, DeleteLast };

// Qt AutoExpandingTreeView sends Backspace/Escape to CollectionFilterWidget::FocusOnFilter via sendEvent.
inline Effect KeyEffect(unsigned keyval) {
  if (keyval == ListBoxKeyboard::kEscape) {
    return Effect::Clear;
  }
  if (keyval == ListBoxKeyboard::kBackSpace) {
    return Effect::DeleteLast;
  }
  return Effect::None;
}

inline std::string DeleteLastUtf8(const std::string &current) {
  if (current.empty()) {
    return current;
  }
  size_t i = current.size();
  do {
    --i;
  } while (i > 0 && (static_cast<unsigned char>(current[i]) & 0x80) != 0 && (static_cast<unsigned char>(current[i]) & 0x40) == 0);
  return current.substr(0, i);
}

inline std::string Apply(const std::string &current, Effect effect) {
  if (effect == Effect::Clear) {
    return {};
  }
  if (effect == Effect::DeleteLast) {
    return DeleteLastUtf8(current);
  }
  return current;
}

}  // namespace CollectionFilterFocus

#endif
