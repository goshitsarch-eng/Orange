#ifndef STRAWBERRY_FILEVIEWMENU_H
#define STRAWBERRY_FILEVIEWMENU_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"
#include "core/song.h"
#include "playlistparsers/playlistparser.h"
#include "utilities/fileutils.h"

#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace FileViewMenu {

enum class Action { Append, Replace, New, Copy, Move, Device, Delete, EditTags, Browse };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::Append;
};

inline std::vector<Item> Items() {
  return {
      {"Append to current playlist", "append", Action::Append},
      {"Replace current playlist", "replace", Action::Replace},
      {"Open in new playlist", "new", Action::New},
      {"Copy to collection…", "copy", Action::Copy},
      {"Move to collection…", "move", Action::Move},
      {"Copy to device…", "device", Action::Device},
      {"Delete from disk…", "delete", Action::Delete},
      {"Edit track information…", "edit-tags", Action::EditTags},
      {"Show in file browser…", "browse", Action::Browse},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Append;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Append;
}

inline bool IsPlaylistAction(Action action) {
  return action == Action::Append || action == Action::Replace || action == Action::New;
}

inline CollectionBehaviour::Plan PlanFor(Action action, BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  switch (action) {
    case Action::Replace:
      return CollectionBehaviour::Replace(play, engine_stopped);
    case Action::New:
      return CollectionBehaviour::OpenInNew(play, engine_stopped);
    case Action::Append:
    default:
      return CollectionBehaviour::Append(play, engine_stopped);
  }
}

// Qt File View selection goes through SongLoader::LoadLocalDirectory, which walks every subdirectory.
inline bool IsLoadableFile(const std::string &path) {
  return PlaylistParser::IsPlaylist(path) || Song::IsAudioFile(path);
}

inline std::vector<std::string> CollectLoadablePaths(const std::string &path) {
  if (path.empty()) {
    return {};
  }
  if (FileUtils::IsDirectory(path)) {
    std::vector<std::string> files;
    for (const std::string &entry : FileUtils::ListDirectoryRecursive(path)) {
      if (IsLoadableFile(entry)) {
        files.push_back(entry);
      }
    }
    return files;
  }
  if (IsLoadableFile(path)) {
    return {path};
  }
  return {};
}

inline std::vector<std::string> ExpandPaths(const std::vector<std::string> &paths) {
  std::vector<std::string> files;
  for (const std::string &path : paths) {
    const std::vector<std::string> collected = CollectLoadablePaths(path);
    files.insert(files.end(), collected.begin(), collected.end());
  }
  return files;
}

// Qt File View playlist actions keep the selected folder; SongLoader walks it. Device/edit-tags still need files.
inline bool ExpandsPaths(Action action) {
  switch (action) {
    case Action::Device:
    case Action::EditTags:
      return true;
    case Action::Append:
    case Action::Replace:
    case Action::Copy:
    case Action::Move:
    case Action::Delete:
    case Action::Browse:
    default:
      return false;
  }
}

inline std::vector<std::string> PathsForAction(Action action, const std::vector<std::string> &selection) {
  return ExpandsPaths(action) ? ExpandPaths(selection) : selection;
}

inline constexpr int kNewPlaylistNameMaxLength = 20;

inline const char *TreeDefaultPlaylistName() { return "Files"; }

// Qt FileViewList/Tree MimeDataFromSelection: one folder uses its path (basename if longer than 20).
// List mode otherwise uses the current root the same way. Tree mode otherwise uses "Files".
inline std::string PathPlaylistName(const std::string &path) {
  if (path.size() <= kNewPlaylistNameMaxLength) {
    return path;
  }
  if (path.empty() || FileUtils::IsDirectory(path)) {
    return FileUtils::BaseName(path);
  }
  const std::string base = FileUtils::BaseName(path);
  const std::string ext = FileUtils::Extension(path);
  if (ext.empty() || ext.size() + 1 >= base.size()) {
    return base;
  }
  return base.substr(0, base.size() - ext.size() - 1);
}

// Qt FileViewTree::mousePressEvent middle-click enqueues the clicked path after selecting it.
inline std::vector<std::string> TreeEnqueuePaths(const std::string &path) {
  return path.empty() ? std::vector<std::string>{} : std::vector<std::string>{path};
}

// Qt FileView::ItemDoubleClick sets MimeData::name_for_new_playlist_ to the file path.
inline std::string DoubleClickPlaylistName(const std::vector<std::string> &paths) {
  return paths.empty() ? std::string() : paths.front();
}

inline std::string NewPlaylistName(const std::vector<std::string> &selection, const std::string &current_path, bool tree_mode) {
  if (selection.size() == 1 && FileUtils::IsDirectory(selection.front())) {
    return PathPlaylistName(selection.front());
  }
  if (tree_mode) {
    return TreeDefaultPlaylistName();
  }
  return PathPlaylistName(current_path);
}

inline std::string BrowserPath(const std::vector<std::string> &paths) {
  if (paths.empty()) {
    return {};
  }
  return paths.front();
}

// Qt OpenInFileBrowser groups by QFileInfo::dir() and opens one window per directory.
inline std::string BrowserDirectory(const std::string &path) { return path.empty() ? std::string() : FileUtils::DirName(path); }

inline std::vector<std::string> BrowserPaths(const std::vector<std::string> &paths) {
  std::vector<std::string> result;
  std::set<std::string> seen_dirs;
  for (const std::string &path : paths) {
    const std::string dir = BrowserDirectory(path);
    if (dir.empty() || !seen_dirs.insert(dir).second) {
      continue;
    }
    result.push_back(path);
  }
  return result;
}

inline constexpr int kBrowserConfirmThreshold = 5;
inline constexpr int kBrowserTooManyThreshold = 50;

enum class BrowserOpenPolicy { Open, Confirm, TooMany };

inline BrowserOpenPolicy BrowserPolicy(int directory_count) {
  if (directory_count > kBrowserTooManyThreshold) {
    return BrowserOpenPolicy::TooMany;
  }
  if (directory_count > kBrowserConfirmThreshold) {
    return BrowserOpenPolicy::Confirm;
  }
  return BrowserOpenPolicy::Open;
}

inline const char *BrowserTooManyMessage() { return "Too many songs selected."; }

inline std::string BrowserConfirmMessage(int song_count, int directory_count) {
  return std::to_string(song_count) + " songs in " + std::to_string(directory_count) +
         " different directories selected, are you sure you want to open them all?";
}

}  // namespace FileViewMenu

#endif
