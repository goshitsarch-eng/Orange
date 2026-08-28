#ifndef STRAWBERRY_SMARTPLAYLISTCONTEXTMENU_H
#define STRAWBERRY_SMARTPLAYLISTCONTEXTMENU_H

#include "smartplaylists/smartplaylistsitem.h"

// Qt SmartPlaylistsView::contextMenuEvent uses currentIndex() when
// QContextMenuEvent::Keyboard (Menu / Shift+F10). Pointer events use indexAt(pos).
// Invalid index opens the empty (wizard-only) menu.

namespace SmartPlaylistContextMenu {

constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

inline const SmartPlaylistsItem *ItemForMenu(bool keyboard, const SmartPlaylistsItem *selected, const SmartPlaylistsItem *at_pointer) {
  return keyboard ? selected : at_pointer;
}

}  // namespace SmartPlaylistContextMenu

#endif  // STRAWBERRY_SMARTPLAYLISTCONTEXTMENU_H
