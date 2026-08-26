#ifndef STRAWBERRY_FILEMANAGERUTILS_H
#define STRAWBERRY_FILEMANAGERUTILS_H

#include <string>

namespace FileManagerUtils {

bool OpenInFileManager(const std::string &path);
bool OpenFolder(const std::string &path);

}  // namespace FileManagerUtils

#endif
