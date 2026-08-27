#ifndef STRAWBERRY_PLAYLISTFILTEREMPTY_H
#define STRAWBERRY_PLAYLISTFILTEREMPTY_H

namespace PlaylistFilterEmpty {

inline bool ShouldShow(int total_rows, int visible_rows) { return total_rows > 0 && visible_rows == 0; }

inline const char *Message() { return "No matches found. Clear the search box to show the whole playlist again."; }

}  // namespace PlaylistFilterEmpty

#endif  // STRAWBERRY_PLAYLISTFILTEREMPTY_H
