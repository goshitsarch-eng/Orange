#ifndef STRAWBERRY_PLAYLISTFILTEREMPTY_H
#define STRAWBERRY_PLAYLISTFILTEREMPTY_H

namespace PlaylistFilterEmpty {

// What, if anything, the playlist should show in place of rows.
enum class State {
  // The playlist has rows and at least one of them is visible.
  None,
  // The playlist has no rows at all.
  EmptyPlaylist,
  // The playlist has rows, but the active filter hides every one of them.
  NoMatches,
};

inline State StateFor(int total_rows, int visible_rows) {
  if (total_rows == 0) {
    return State::EmptyPlaylist;
  }
  if (visible_rows == 0) {
    return State::NoMatches;
  }
  return State::None;
}

inline bool ShouldShow(int total_rows, int visible_rows) { return StateFor(total_rows, visible_rows) != State::None; }

inline const char *Message() { return "No matches found. Clear the search box to show the whole playlist again."; }

inline const char *NoMatchesTitle() { return "No matching tracks"; }
inline const char *NoMatchesDescription() { return "Clear the filter to show the whole playlist again."; }
inline const char *NoMatchesIcon() { return "system-search-symbolic"; }

inline const char *EmptyTitle() { return "This playlist is empty"; }
inline const char *EmptyDescription() { return "Add tracks from your collection, or drag music here from your file manager."; }
inline const char *EmptyIcon() { return "view-list-symbolic"; }

inline const char *TitleFor(State state) { return state == State::NoMatches ? NoMatchesTitle() : EmptyTitle(); }
inline const char *DescriptionFor(State state) { return state == State::NoMatches ? NoMatchesDescription() : EmptyDescription(); }
inline const char *IconFor(State state) { return state == State::NoMatches ? NoMatchesIcon() : EmptyIcon(); }

}  // namespace PlaylistFilterEmpty

#endif  // STRAWBERRY_PLAYLISTFILTEREMPTY_H
