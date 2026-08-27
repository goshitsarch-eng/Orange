#ifndef STRAWBERRY_FILEVIEWNAV_H
#define STRAWBERRY_FILEVIEWNAV_H

#include "fileview/fileviewmode.h"
#include "utilities/fileutils.h"

#include <string>

namespace FileViewNav {

// Qt FileView::FileUp: ui_->up->setEnabled(dir.cdUp()).
inline bool UpEnabled(const std::string &path) {
  const std::string clean = FileViewMode::CleanPath(path);
  if (clean.empty() || clean == "/") {
    return false;
  }
  return FileViewMode::CleanPath(FileUtils::DirName(clean)) != clean;
}

}  // namespace FileViewNav

#endif
