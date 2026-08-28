#ifndef STRAWBERRY_PLAYLISTROWINDEX_H
#define STRAWBERRY_PLAYLISTROWINDEX_H

#include <glib-object.h>

// Row widgets carry their playlist row index as object data.  It is stored one-based because
// GINT_TO_POINTER(0) is NULL, which g_object_get_data() cannot tell apart from "this key was never set":
// with a zero-based index the column header, which carries no index at all, read back as row 0, so editing
// the first row rewrote the header instead.
namespace PlaylistRowIndex {

inline constexpr const char *kKey = "row-index";

inline void Set(gpointer widget, int index) { g_object_set_data(G_OBJECT(widget), kKey, GINT_TO_POINTER(index + 1)); }

// The row index, or -1 for a widget that is not a row.
inline int Get(gpointer widget) {
  const int stored = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), kKey));
  return stored > 0 ? stored - 1 : -1;
}

}  // namespace PlaylistRowIndex

#endif
