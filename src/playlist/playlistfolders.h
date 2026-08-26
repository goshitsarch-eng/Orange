#ifndef STRAWBERRY_PLAYLISTFOLDERS_H
#define STRAWBERRY_PLAYLISTFOLDERS_H

#include "playlist/playlistlistdrop.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace PlaylistFolders {

struct PlaylistRef {
  std::string name;
  bool favorite = false;
  std::string ui_path;
};

inline std::string SanitizeName(std::string name) {
  for (char &ch : name) {
    if (ch == '/') {
      ch = ' ';
    }
  }
  return name;
}

inline std::vector<std::string> Split(const std::string &path) {
  std::vector<std::string> parts;
  std::string current;
  for (char ch : path) {
    if (ch == '/') {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    parts.push_back(current);
  }
  return parts;
}

inline std::string Join(const std::vector<std::string> &parts) {
  std::string out;
  for (const std::string &part : parts) {
    if (part.empty()) {
      continue;
    }
    if (!out.empty()) {
      out += '/';
    }
    out += part;
  }
  return out;
}

inline std::string Parent(const std::string &path) {
  const auto parts = Split(path);
  if (parts.size() <= 1) {
    return {};
  }
  return Join(std::vector<std::string>(parts.begin(), parts.end() - 1));
}

inline std::string Leaf(const std::string &path) {
  const auto parts = Split(path);
  return parts.empty() ? std::string() : parts.back();
}

inline std::string Child(const std::string &parent, const std::string &name) {
  const std::string clean = SanitizeName(name);
  if (clean.empty()) {
    return parent;
  }
  if (parent.empty()) {
    return clean;
  }
  return parent + "/" + clean;
}

inline bool IsUnder(const std::string &path, const std::string &folder) {
  if (folder.empty()) {
    return true;
  }
  if (path == folder) {
    return true;
  }
  return path.size() > folder.size() && path.compare(0, folder.size(), folder) == 0 && path[folder.size()] == '/';
}

inline std::string RenamePrefix(const std::string &path, const std::string &old_folder, const std::string &new_folder) {
  if (path == old_folder) {
    return new_folder;
  }
  if (old_folder.empty() || path.size() <= old_folder.size()) {
    return path;
  }
  if (path.compare(0, old_folder.size(), old_folder) == 0 && path[old_folder.size()] == '/') {
    return new_folder + path.substr(old_folder.size());
  }
  return path;
}

inline std::set<std::string> CollectFolders(const std::vector<PlaylistRef> &playlists, const std::vector<std::string> &extra) {
  std::set<std::string> folders;
  auto add = [&](const std::string &path) {
    const auto parts = Split(path);
    std::string built;
    for (const std::string &part : parts) {
      built = Child(built, part);
      if (!built.empty()) {
        folders.insert(built);
      }
    }
  };
  for (const PlaylistRef &playlist : playlists) {
    add(playlist.ui_path);
  }
  for (const std::string &folder : extra) {
    add(folder);
  }
  return folders;
}

inline std::vector<std::string> ParseFolderList(const std::string &text) {
  std::vector<std::string> folders;
  std::string current;
  for (char ch : text) {
    if (ch == '\n') {
      if (!current.empty()) {
        folders.push_back(current);
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    folders.push_back(current);
  }
  return folders;
}

inline std::string JoinFolderList(const std::vector<std::string> &folders) {
  std::string text;
  for (const std::string &folder : folders) {
    if (folder.empty()) {
      continue;
    }
    if (!text.empty()) {
      text += '\n';
    }
    text += folder;
  }
  return text;
}

inline std::vector<PlaylistListDrop::Row> Flatten(const std::vector<PlaylistRef> &playlists, const std::vector<std::string> &extra_folders,
                                                 const std::set<std::string> &collapsed, const std::string &filter, bool favorites_only) {
  std::vector<PlaylistRef> visible;
  for (const PlaylistRef &playlist : playlists) {
    if (favorites_only && !playlist.favorite) {
      continue;
    }
    if (!filter.empty() && !StrUtils::ContainsInsensitive(playlist.name, filter)) {
      continue;
    }
    visible.push_back(playlist);
  }
  const bool filtering = !filter.empty();
  const std::set<std::string> folders = CollectFolders(visible, filtering ? std::vector<std::string>{} : extra_folders);
  const std::vector<std::string> folder_list(folders.begin(), folders.end());
  std::sort(visible.begin(), visible.end(), [](const PlaylistRef &a, const PlaylistRef &b) { return a.name < b.name; });

  struct Child {
    bool folder = false;
    std::string name;
    std::string path;
    PlaylistRef playlist;
  };

  std::vector<PlaylistListDrop::Row> rows;
  struct Walker {
    const std::vector<std::string> *folder_list = nullptr;
    const std::vector<PlaylistRef> *visible = nullptr;
    const std::set<std::string> *collapsed = nullptr;
    bool filtering = false;
    std::vector<PlaylistListDrop::Row> *rows = nullptr;

    void Walk(const std::string &parent, int depth) const {
      std::vector<Child> children;
      for (const std::string &folder : *folder_list) {
        if (Parent(folder) == parent) {
          children.push_back({true, Leaf(folder), folder, {}});
        }
      }
      for (const PlaylistRef &playlist : *visible) {
        if (playlist.ui_path == parent) {
          children.push_back({false, playlist.name, playlist.ui_path, playlist});
        }
      }
      std::sort(children.begin(), children.end(), [](const Child &a, const Child &b) { return a.name < b.name; });
      for (const Child &child : children) {
        PlaylistListDrop::Row row;
        row.name = child.name;
        row.favorite = child.playlist.favorite;
        row.folder = child.folder;
        row.path = child.path;
        row.depth = depth;
        row.expanded = child.folder && (filtering || collapsed->count(child.path) == 0);
        rows->push_back(row);
        if (child.folder && row.expanded) {
          Walk(child.path, depth + 1);
        }
      }
    }
  };
  Walker walker{&folder_list, &visible, &collapsed, filtering, &rows};
  walker.Walk("", 0);
  return rows;
}

}  // namespace PlaylistFolders

#endif
