#ifndef STRAWBERRY_COLLECTIONCONTEXTMENU_H
#define STRAWBERRY_COLLECTIONCONTEXTMENU_H

// Qt CollectionView::contextMenuEvent returns without a menu when
// indexAt(pos) is invalid. Keyboard Menu / Shift+F10 uses the current
// selection the same way PlaylistView does.

namespace CollectionContextMenu {

constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

inline bool ShouldShowMenu(bool has_selection) { return has_selection; }

}  // namespace CollectionContextMenu

#endif  // STRAWBERRY_COLLECTIONCONTEXTMENU_H
