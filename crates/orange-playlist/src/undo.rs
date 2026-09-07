//! Undoable playlist editing.
//! Mirrors `playlistundocommand{base,insert,move,remove,reorder,shuffle,sort}`:
//! every edit is a command enum applied to a [`Playlist`](crate::model::Playlist).

use crate::model::{Playlist, PlaylistItem};

/// One undoable edit.
#[derive(Debug, Clone)]
pub enum UndoCommand {
    Insert {
        index: usize,
        items: Vec<PlaylistItem>,
    },
    Remove {
        index: usize,
        count: usize,
    },
    Move {
        from: usize,
        to: usize,
    },
}

#[derive(Debug, Clone)]
struct Applied {
    command: UndoCommand,
    /// Rows captured so the command can be undone.
    removed: Vec<PlaylistItem>,
}

/// Bounded undo/redo stack.
#[derive(Debug, Default)]
pub struct UndoStack {
    undo: Vec<Applied>,
    redo: Vec<Applied>,
    limit: usize,
}

impl UndoStack {
    pub fn new(limit: usize) -> Self {
        Self {
            undo: Vec::new(),
            redo: Vec::new(),
            limit: limit.max(1),
        }
    }

    pub fn can_undo(&self) -> bool {
        !self.undo.is_empty()
    }

    pub fn can_redo(&self) -> bool {
        !self.redo.is_empty()
    }

    pub fn apply(&mut self, list: &mut Playlist, command: UndoCommand) {
        let removed = execute(list, &command);
        self.redo.clear();
        self.undo.push(Applied { command, removed });
        if self.undo.len() > self.limit {
            self.undo.remove(0);
        }
    }

    pub fn undo(&mut self, list: &mut Playlist) -> bool {
        let Some(applied) = self.undo.pop() else {
            return false;
        };
        revert(list, &applied);
        self.redo.push(applied);
        true
    }

    pub fn redo(&mut self, list: &mut Playlist) -> bool {
        let Some(applied) = self.redo.pop() else {
            return false;
        };
        let removed = execute(list, &applied.command);
        self.undo.push(Applied {
            command: applied.command,
            removed,
        });
        true
    }
}

fn execute(list: &mut Playlist, command: &UndoCommand) -> Vec<PlaylistItem> {
    match command {
        UndoCommand::Insert { index, items } => {
            for (offset, item) in items.iter().enumerate() {
                list.insert(index + offset, item.clone());
            }
            Vec::new()
        }
        UndoCommand::Remove { index, count } => {
            let mut removed = Vec::new();
            for _ in 0..*count {
                if let Some(item) = list.remove(*index) {
                    removed.push(item);
                } else {
                    break;
                }
            }
            removed
        }
        UndoCommand::Move { from, to } => {
            list.move_row(*from, *to);
            Vec::new()
        }
    }
}

fn revert(list: &mut Playlist, applied: &Applied) {
    match &applied.command {
        UndoCommand::Insert { index, items } => {
            for _ in 0..items.len() {
                list.remove(*index);
            }
        }
        UndoCommand::Remove { index, .. } => {
            for (offset, item) in applied.removed.iter().enumerate() {
                list.insert(index + offset, item.clone());
            }
        }
        UndoCommand::Move { from, to } => {
            list.move_row(*to, *from);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use orange_core::song::Song;

    fn song(title: &str) -> PlaylistItem {
        PlaylistItem::Song(Box::new(Song {
            title: title.into(),
            url: title.into(),
            ..Default::default()
        }))
    }

    fn titles(list: &Playlist) -> Vec<String> {
        list.items().iter().map(|i| i.title().to_string()).collect()
    }

    #[test]
    fn insert_undo_redo() {
        let mut list = Playlist::new();
        let mut stack = UndoStack::new(10);
        stack.apply(
            &mut list,
            UndoCommand::Insert {
                index: 0,
                items: vec![song("a"), song("b")],
            },
        );
        assert_eq!(titles(&list), ["a", "b"]);
        assert!(stack.undo(&mut list));
        assert!(list.is_empty());
        assert!(stack.redo(&mut list));
        assert_eq!(titles(&list), ["a", "b"]);
    }

    #[test]
    fn remove_restores_rows() {
        let mut list = Playlist::new();
        let mut stack = UndoStack::new(10);
        stack.apply(
            &mut list,
            UndoCommand::Insert {
                index: 0,
                items: vec![song("a"), song("b")],
            },
        );
        stack.apply(&mut list, UndoCommand::Remove { index: 0, count: 1 });
        assert_eq!(titles(&list), ["b"]);
        assert!(stack.undo(&mut list));
        assert_eq!(titles(&list), ["a", "b"]);
    }

    #[test]
    fn redo_cleared_by_new_command() {
        let mut list = Playlist::new();
        let mut stack = UndoStack::new(10);
        stack.apply(
            &mut list,
            UndoCommand::Insert {
                index: 0,
                items: vec![song("a")],
            },
        );
        assert!(stack.undo(&mut list));
        stack.apply(
            &mut list,
            UndoCommand::Insert {
                index: 0,
                items: vec![song("z")],
            },
        );
        assert!(!stack.can_redo());
        assert_eq!(titles(&list), ["z"]);
    }

    #[test]
    fn empty_stack_is_noop() {
        let mut list = Playlist::new();
        let mut stack = UndoStack::new(10);
        assert!(!stack.undo(&mut list));
        assert!(!stack.redo(&mut list));
    }
}
