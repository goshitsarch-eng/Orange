#include "fileview/fileviewtreemodel.h"

#include "core/song.h"
#include "fileview/fileviewhidden.h"
#include "playlistparsers/playlistparser.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>

void FileViewTreeModel::SetRootPaths(const std::vector<std::string> &paths) {
  Reset();
  root_ = std::make_unique<FileViewTreeItem>();
  root_->type = FileViewTreeItem::Type::Root;
  root_->name = "Computer";
  root_->loaded = true;
  for (const std::string &path : paths) {
    if (!FileUtils::IsDirectory(path)) {
      continue;
    }
    auto child = std::make_unique<FileViewTreeItem>();
    child->path = path;
    child->name = FileUtils::BaseName(path).empty() ? path : FileUtils::BaseName(path);
    child->type = FileViewTreeItem::Type::Directory;
    child->parent = root_.get();
    root_->children.push_back(std::move(child));
  }
}

void FileViewTreeModel::SetNameFilters(const std::vector<std::string> &filters) { name_filters_ = filters; }

void FileViewTreeModel::SetShowHidden(bool show_hidden) { show_hidden_ = show_hidden; }

void FileViewTreeModel::SetShowAllFiles(bool show_all) { show_all_files_ = show_all; }

void FileViewTreeModel::Reset() { root_.reset(); }

bool FileViewTreeModel::AcceptsFile(const std::string &path) const {
  if (show_all_files_) {
    return true;
  }
  if (Song::IsAudioFile(path) || PlaylistParser::IsPlaylist(path)) {
    return true;
  }
  if (name_filters_.empty()) {
    return false;
  }
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  return std::find(name_filters_.begin(), name_filters_.end(), ext) != name_filters_.end();
}

void FileViewTreeModel::LazyLoad(FileViewTreeItem *item) {
  if (!item || item->loaded || item->type == FileViewTreeItem::Type::File) {
    return;
  }
  item->loaded = true;
  std::vector<std::string> entries = FileUtils::ListDirectory(item->path);
  std::sort(entries.begin(), entries.end());
  for (const std::string &path : entries) {
    const std::string name = FileUtils::BaseName(path);
    if (!FileViewHidden::ShouldIncludeEntry(name, show_hidden_)) {
      continue;
    }
    if (!FileUtils::IsDirectory(path)) {
      continue;
    }
    auto child = std::make_unique<FileViewTreeItem>();
    child->path = path;
    child->name = name;
    child->type = FileViewTreeItem::Type::Directory;
    child->parent = item;
    item->children.push_back(std::move(child));
  }
}

int FileViewTreeModel::DirectoryCount() const {
  if (!root_) {
    return 0;
  }
  return static_cast<int>(root_->children.size());
}

std::vector<std::string> FileViewTreeModel::FilesIn(const std::string &directory) const {
  std::vector<std::string> files;
  std::vector<std::string> entries = FileUtils::ListDirectory(directory);
  std::sort(entries.begin(), entries.end());
  for (const std::string &path : entries) {
    const std::string name = FileUtils::BaseName(path);
    if (!FileViewHidden::ShouldIncludeEntry(name, show_hidden_)) {
      continue;
    }
    if (FileUtils::IsDirectory(path) || AcceptsFile(path)) {
      files.push_back(path);
    }
  }
  return files;
}
