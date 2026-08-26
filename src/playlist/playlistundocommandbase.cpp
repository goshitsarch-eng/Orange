#include "playlist/playlistundocommandbase.h"

PlaylistUndoCommandInsertItems::PlaylistUndoCommandInsertItems(int row, const SongList &songs) : row_(row), songs_(songs) {}

PlaylistUndoCommandRemoveItems::PlaylistUndoCommandRemoveItems(const std::vector<int> &rows, const SongList &songs)
    : rows_(rows), songs_(songs) {}

PlaylistUndoCommandMoveItems::PlaylistUndoCommandMoveItems(int from, int to) : from_(from), to_(to) {}

PlaylistUndoCommandReorderItems::PlaylistUndoCommandReorderItems(const std::vector<int> &order) : order_(order) {}

PlaylistUndoCommandSortItems::PlaylistUndoCommandSortItems(int column, bool descending) : column_(column), descending_(descending) {}
