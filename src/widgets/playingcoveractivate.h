#ifndef STRAWBERRY_PLAYINGCOVERACTIVATE_H
#define STRAWBERRY_PLAYINGCOVERACTIVATE_H

namespace PlayingCoverActivate {

// Qt PlayingWidget::mouseDoubleClickEvent: LeftButton && song_.is_valid()
inline bool ShouldShow(bool primary, int n_press, bool song_valid) { return primary && n_press == 2 && song_valid; }

// Qt ContextAlbum::mouseDoubleClickEvent also requires a real cover (not the placeholder).
inline bool ShouldShowContext(bool primary, int n_press, bool song_valid, bool has_cover) {
  return ShouldShow(primary, n_press, song_valid) && has_cover;
}

}  // namespace PlayingCoverActivate

#endif
