#ifndef STRAWBERRY_FILEUTILS_H
#define STRAWBERRY_FILEUTILS_H

#include <cstdint>
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
std::vector<std::string> ListDirectoryRecursive(const std::string &path);
std::string PathFromUri(const std::string &uri);
std::string UriFromPath(const std::string &path);
std::string ReadFile(const std::string &path);
bool WriteFile(const std::string &path, const std::string &contents);
bool CopyFile(const std::string &source, const std::string &destination);
bool Remove(const std::string &path);
bool RemoveRecursive(const std::string &path);
bool MoveToTrash(const std::string &path);
std::string PrettySize(int64_t bytes);
int64_t FreeSpaceBytes(const std::string &path);
int64_t TotalSpaceBytes(const std::string &path);
int64_t FileSize(const std::string &path);
int64_t FileMtime(const std::string &path);
bool FilenameOnGVFS(const std::string &path);

}  // namespace FileUtils

#endif
