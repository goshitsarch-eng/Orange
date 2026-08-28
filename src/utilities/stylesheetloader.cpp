#include "utilities/stylesheetloader.h"

#include "utilities/fileutils.h"
#include "utilities/styleutils.h"

bool StyleSheetLoader::LoadFile(const std::string &path) {
  const std::string css = FileUtils::ReadFile(path);
  if (css.empty()) {
    return false;
  }
  StyleUtils::LoadCss(css, StyleUtils::Slot::kUserStyleSheet);
  return true;
}
