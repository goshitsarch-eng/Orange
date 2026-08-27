#ifndef STRAWBERRY_COLLECTIONFOCUS_H
#define STRAWBERRY_COLLECTIONFOCUS_H

#include "collection/collectionitem.h"
#include "collection/collectiontree.h"
#include "core/song.h"

#include <set>
#include <string>

namespace CollectionFocus {

struct State {
  Song last_selected_song;
  std::string last_selected_container;
  std::set<std::string> last_selected_path;
};

inline bool ShouldSave(CollectionItem::Type type) {
  return type == CollectionItem::Type::Song || type == CollectionItem::Type::Container || type == CollectionItem::Type::Divider;
}

inline bool ShouldRestore(const State &state) {
  return !state.last_selected_container.empty() || !state.last_selected_song.url().empty();
}

inline Song SongFromItem(const CollectionItem *item) {
  if (!item) {
    return Song();
  }
  const SongList songs = item->Songs();
  return songs.empty() ? Song() : songs.back();
}

inline bool SongMatches(const CollectionItem *item, const Song &wanted) {
  if (!item || wanted.url().empty()) {
    return false;
  }
  for (const Song &song : item->Songs()) {
    if (song == wanted) {
      return true;
    }
  }
  return false;
}

inline void SaveContainerPath(const CollectionItem *child, State *state) {
  if (!child || !state || !child->parent) {
    return;
  }
  const CollectionItem *current = child->parent;
  if (current->type != CollectionItem::Type::Container && current->type != CollectionItem::Type::Divider) {
    return;
  }
  if (!current->sort_text.empty()) {
    state->last_selected_path.insert(current->sort_text);
  }
  SaveContainerPath(current, state);
}

inline void Capture(const CollectionItem *item, State *state) {
  if (!state || !item || !ShouldSave(item->type)) {
    return;
  }
  state->last_selected_path.clear();
  state->last_selected_song = Song();
  state->last_selected_container.clear();
  if (item->type == CollectionItem::Type::Song) {
    state->last_selected_song = SongFromItem(item);
  } else {
    state->last_selected_container = item->sort_text;
  }
  SaveContainerPath(item, state);
}

inline const CollectionItem *RestoreLevel(const CollectionItem *parent, const State &state, std::set<std::string> *expand_keys) {
  if (!parent) {
    return nullptr;
  }
  for (const auto &child : parent->children) {
    const CollectionItem *current = child.get();
    if (!current) {
      continue;
    }
    switch (current->type) {
      case CollectionItem::Type::Root:
      case CollectionItem::Type::LoadingIndicator:
        break;
      case CollectionItem::Type::Song:
        if (SongMatches(current, state.last_selected_song)) {
          return current;
        }
        break;
      case CollectionItem::Type::Container:
      case CollectionItem::Type::Divider: {
        if (!state.last_selected_container.empty() && state.last_selected_container == current->sort_text) {
          if (expand_keys && CollectionTree::IsExpandable(current)) {
            expand_keys->insert(CollectionTree::Key(current));
          }
          return current;
        }
        if (state.last_selected_path.count(current->sort_text) > 0) {
          if (expand_keys && CollectionTree::IsExpandable(current)) {
            expand_keys->insert(CollectionTree::Key(current));
          }
          if (const CollectionItem *found = RestoreLevel(current, state, expand_keys)) {
            return found;
          }
          if (expand_keys) {
            expand_keys->erase(CollectionTree::Key(current));
          }
        }
        break;
      }
    }
  }
  return nullptr;
}

inline const CollectionItem *FindTarget(const CollectionItem *root, const State &state) { return RestoreLevel(root, state, nullptr); }

inline std::set<std::string> ExpandKeys(const CollectionItem *root, const State &state) {
  std::set<std::string> keys;
  RestoreLevel(root, state, &keys);
  return keys;
}

inline bool NeedsExpand(const std::set<std::string> &expanded, const std::set<std::string> &needed) {
  for (const std::string &key : needed) {
    if (expanded.find(key) == expanded.end()) {
      return true;
    }
  }
  return false;
}

inline void MergeExpand(std::set<std::string> *expanded, const std::set<std::string> &needed) {
  if (!expanded) {
    return;
  }
  expanded->insert(needed.begin(), needed.end());
}

}  // namespace CollectionFocus

#endif  // STRAWBERRY_COLLECTIONFOCUS_H
