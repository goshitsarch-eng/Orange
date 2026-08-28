#ifndef STRAWBERRY_PLAYINGCOVERACTIVATE_H
#define STRAWBERRY_PLAYINGCOVERACTIVATE_H

namespace PlayingCoverActivate {

// Qt PlayingWidget::mouseDoubleClickEvent: LeftButton && song_.is_valid()
inline bool ShouldShow(bool primary, int n_press, bool song_valid) { return primary && n_press == 2 && song_valid; }

// Qt ContextAlbum::mouseDoubleClickEvent also requires a real cover (not the placeholder).
inline bool ShouldShowContext(bool primary, int n_press, bool song_valid, bool has_cover) {
  return ShouldShow(primary, n_press, song_valid) && has_cover;
}

// Qt PlayingWidget::contextMenuEvent always pops from Menu / Shift+F10.
constexpr unsigned kMenu = 0xff67;
constexpr unsigned kF10 = 0xffc7;
constexpr unsigned kShiftMask = 1u << 0;
constexpr double kKeyboardY = -1;

inline bool IsKeyboardTrigger(unsigned keyval, unsigned state) {
  return keyval == kMenu || (keyval == kF10 && (state & kShiftMask) != 0);
}

inline bool ShouldShowMenu() { return true; }

inline bool IsKeyboardAnchor(double y) { return y < 0; }

}  // namespace PlayingCoverActivate

#endif
