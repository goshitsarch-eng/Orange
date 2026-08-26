#ifndef STRAWBERRY_PLAYLISTUNDOCOMMANDBASE_H
#define STRAWBERRY_PLAYLISTUNDOCOMMANDBASE_H

#include "core/song.h"

#include <string>
#include <vector>

class PlaylistUndoCommandBase {
 public:
  enum class Type {
    InsertItems,
    RemoveItems,
    MoveItems,
    ReorderItems,
    ShuffleItems,
    SortItems
  };

  virtual ~PlaylistUndoCommandBase() = default;
  virtual Type type() const = 0;
  virtual std::string text() const = 0;
};

class PlaylistUndoCommandInsertItems : public PlaylistUndoCommandBase {
 public:
  PlaylistUndoCommandInsertItems(int row, const SongList &songs);
  Type type() const override { return Type::InsertItems; }
  std::string text() const override { return "Insert items"; }
  int row() const { return row_; }
  const SongList &songs() const { return songs_; }

 private:
  int row_ = 0;
  SongList songs_;
};

class PlaylistUndoCommandRemoveItems : public PlaylistUndoCommandBase {
 public:
  PlaylistUndoCommandRemoveItems(const std::vector<int> &rows, const SongList &songs);
  Type type() const override { return Type::RemoveItems; }
  std::string text() const override { return "Remove items"; }
  const std::vector<int> &rows() const { return rows_; }
  const SongList &songs() const { return songs_; }

 private:
  std::vector<int> rows_;
  SongList songs_;
};

class PlaylistUndoCommandMoveItems : public PlaylistUndoCommandBase {
 public:
  PlaylistUndoCommandMoveItems(int from, int to);
  Type type() const override { return Type::MoveItems; }
  std::string text() const override { return "Move items"; }
  int from() const { return from_; }
  int to() const { return to_; }

 private:
  int from_ = 0;
  int to_ = 0;
};

class PlaylistUndoCommandReorderItems : public PlaylistUndoCommandBase {
 public:
  explicit PlaylistUndoCommandReorderItems(const std::vector<int> &order);
  Type type() const override { return Type::ReorderItems; }
  std::string text() const override { return "Reorder items"; }
  const std::vector<int> &order() const { return order_; }

 private:
  std::vector<int> order_;
};

class PlaylistUndoCommandShuffleItems : public PlaylistUndoCommandBase {
 public:
  Type type() const override { return Type::ShuffleItems; }
  std::string text() const override { return "Shuffle items"; }
};

class PlaylistUndoCommandSortItems : public PlaylistUndoCommandBase {
 public:
  explicit PlaylistUndoCommandSortItems(int column, bool descending);
  Type type() const override { return Type::SortItems; }
  std::string text() const override { return "Sort items"; }
  int column() const { return column_; }
  bool descending() const { return descending_; }

 private:
  int column_ = 0;
  bool descending_ = false;
};

#endif
