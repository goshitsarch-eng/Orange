#ifndef STRAWBERRY_PLAYLISTUNDOSTATE_H
#define STRAWBERRY_PLAYLISTUNDOSTATE_H

namespace PlaylistUndoState {

inline bool UndoEnabled(bool can_undo) { return can_undo; }
inline bool RedoEnabled(bool can_redo) { return can_redo; }
inline const char *UndoTooltip(bool) { return "Undo"; }
inline const char *RedoTooltip(bool) { return "Redo"; }

}  // namespace PlaylistUndoState

#endif  // STRAWBERRY_PLAYLISTUNDOSTATE_H
