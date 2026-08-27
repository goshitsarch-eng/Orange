#ifndef STRAWBERRY_PLAYLISTLISTCONTEXTMENU_H
#define STRAWBERRY_PLAYLISTLISTCONTEXTMENU_H

// Qt PlaylistListContainer::contextMenuEvent always pops the sidebar menu
// (New folder / Save / Copy to device), including when no row is selected.
// Keyboard Menu / Shift+F10 uses the current selection name, which may be empty.

namespace PlaylistListContextMenu {

constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;

inline bool IsTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

inline bool ShouldShowMenu() { return true; }

}  // namespace PlaylistListContextMenu

#endif  // STRAWBERRY_PLAYLISTLISTCONTEXTMENU_H
