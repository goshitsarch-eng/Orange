#ifndef STRAWBERRY_FILEVIEWTREEITEM_H
#define STRAWBERRY_FILEVIEWTREEITEM_H

#include <memory>
#include <string>
#include <vector>

class FileViewTreeItem {
 public:
  enum class Type { Root, Directory, File };

  std::string path;
  std::string name;
  Type type = Type::Directory;
  bool loaded = false;
  FileViewTreeItem *parent = nullptr;
  std::vector<std::unique_ptr<FileViewTreeItem>> children;
};

#endif
