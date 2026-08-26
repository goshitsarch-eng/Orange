#ifndef STRAWBERRY_FILEVIEWMODE_H
#define STRAWBERRY_FILEVIEWMODE_H

#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace FileViewMode {

enum class Mode { List, Tree };

inline Mode DefaultMode() { return Mode::List; }

inline Mode FromTreeActive(bool tree_view_active) { return tree_view_active ? Mode::Tree : Mode::List; }

inline bool TreeActive(Mode mode) { return mode == Mode::Tree; }

inline Mode Toggle(Mode mode) { return mode == Mode::Tree ? Mode::List : Mode::Tree; }

inline bool NavVisible(Mode mode) { return mode == Mode::List; }

inline bool RootButtonsVisible(Mode mode) { return mode == Mode::Tree; }

inline bool ActivateNavigates(Mode mode, bool is_directory) { return mode == Mode::List && is_directory; }

inline bool ActivateAddsToPlaylist(Mode mode, bool is_directory) { return !is_directory && (mode == Mode::List || mode == Mode::Tree); }

inline std::string CleanPath(std::string path) {
  while (path.size() > 1 && (path.back() == '/' || path.back() == '\\')) {
    path.pop_back();
  }
  return path;
}

inline bool SamePath(const std::string &a, const std::string &b) { return CleanPath(a) == CleanPath(b); }

inline bool PathUnderRoot(const std::string &path, const std::string &root) {
  const std::string clean_path = CleanPath(path);
  const std::string clean_root = CleanPath(root);
  if (clean_path.empty() || clean_root.empty()) {
    return false;
  }
  if (clean_path == clean_root) {
    return true;
  }
  return StrUtils::StartsWith(clean_path, clean_root + "/");
}

inline bool ContainsRoot(const std::vector<std::string> &roots, const std::string &path) {
  for (const std::string &root : roots) {
    if (SamePath(root, path)) {
      return true;
    }
  }
  return false;
}

inline std::vector<std::string> AddRoot(std::vector<std::string> roots, const std::string &path) {
  if (path.empty() || ContainsRoot(roots, path)) {
    return roots;
  }
  roots.push_back(path);
  return roots;
}

inline std::string MatchingRoot(const std::vector<std::string> &roots, const std::string &selected) {
  for (const std::string &root : roots) {
    if (PathUnderRoot(selected, root)) {
      return root;
    }
  }
  return {};
}

inline std::vector<std::string> RemoveMatchingRoot(std::vector<std::string> roots, const std::string &selected) {
  const std::string match = MatchingRoot(roots, selected);
  if (match.empty()) {
    return roots;
  }
  std::vector<std::string> next;
  next.reserve(roots.size());
  for (const std::string &root : roots) {
    if (!SamePath(root, match)) {
      next.push_back(root);
    }
  }
  return next;
}

inline std::string EncodeRoots(const std::vector<std::string> &roots) { return StrUtils::Join(roots, "\n"); }

inline std::vector<std::string> DecodeRoots(const std::string &value) {
  std::vector<std::string> roots;
  for (const std::string &part : StrUtils::Split(value, '\n')) {
    const std::string path = StrUtils::Trim(part);
    if (!path.empty()) {
      roots.push_back(path);
    }
  }
  return roots;
}

inline std::vector<std::string> DefaultRoots(const std::string &music) {
  if (music.empty()) {
    return {};
  }
  return {music};
}

inline std::vector<std::string> MenuPaths(const std::vector<std::string> &selected, const std::string &clicked) {
  if (clicked.empty()) {
    return selected;
  }
  for (const std::string &path : selected) {
    if (path == clicked) {
      return selected;
    }
  }
  return {clicked};
}

inline bool ReplaceSelection(bool clicked_selected) { return !clicked_selected; }

}  // namespace FileViewMode

#endif
