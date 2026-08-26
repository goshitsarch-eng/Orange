#ifndef STRAWBERRY_FILEUTILS_H
#define STRAWBERRY_FILEUTILS_H

#include <string>
#include <vector>

namespace FileUtils {

std::string BaseName(const std::string &path);
std::string DirName(const std::string &path);
std::string Extension(const std::string &path);
std::string Join(const std::string &a, const std::string &b);
std::string CanonicalPath(const std::string &path);
bool Exists(const std::string &path);
bool IsDirectory(const std::string &path);
bool IsFile(const std::string &path);
std::vector<std::string> ListDirectory(const std::string &path);
std::string PathFromUri(const std::string &uri);
std::string UriFromPath(const std::string &path);
std::string ReadFile(const std::string &path);
bool WriteFile(const std::string &path, const std::string &contents);

}  // namespace FileUtils

#endif
