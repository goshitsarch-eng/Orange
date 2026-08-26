#include "utilities/fileutils.h"

#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <fstream>
#include <sstream>

namespace FileUtils {

std::string BaseName(const std::string &path) {
  gchar *base = g_path_get_basename(path.c_str());
  std::string result = base ? base : path;
  g_free(base);
  return result;
}

std::string DirName(const std::string &path) {
  gchar *dir = g_path_get_dirname(path.c_str());
  std::string result = dir ? dir : ".";
  g_free(dir);
  return result;
}

std::string Extension(const std::string &path) {
  const std::string base = BaseName(path);
  const auto pos = base.rfind('.');
  if (pos == std::string::npos || pos == 0 || pos + 1 == base.size()) {
    return {};
  }
  return base.substr(pos + 1);
}

std::string Join(const std::string &a, const std::string &b) {
  gchar *joined = g_build_filename(a.c_str(), b.c_str(), nullptr);
  std::string result = joined ? joined : a + "/" + b;
  g_free(joined);
  return result;
}

std::string CanonicalPath(const std::string &path) {
  gchar *canon = g_canonicalize_filename(path.c_str(), nullptr);
  std::string result = canon ? canon : path;
  g_free(canon);
  return result;
}

bool Exists(const std::string &path) { return g_file_test(path.c_str(), G_FILE_TEST_EXISTS); }

bool IsDirectory(const std::string &path) { return g_file_test(path.c_str(), G_FILE_TEST_IS_DIR); }

bool IsFile(const std::string &path) { return g_file_test(path.c_str(), G_FILE_TEST_IS_REGULAR); }

std::vector<std::string> ListDirectory(const std::string &path) {
  std::vector<std::string> result;
  GError *error = nullptr;
  GDir *dir = g_dir_open(path.c_str(), 0, &error);
  if (!dir) {
    if (error) {
      g_error_free(error);
    }
    return result;
  }
  const gchar *name = nullptr;
  while ((name = g_dir_read_name(dir))) {
    result.push_back(Join(path, name));
  }
  g_dir_close(dir);
  return result;
}

std::vector<std::string> ListDirectoryRecursive(const std::string &path) {
  std::vector<std::string> result;
  std::vector<std::string> stack = {path};
  while (!stack.empty()) {
    const std::string dir = stack.back();
    stack.pop_back();
    for (const std::string &entry : ListDirectory(dir)) {
      const std::string name = BaseName(entry);
      if (name.empty() || name == "." || name == ".." || name[0] == '.') {
        continue;
      }
      if (IsDirectory(entry)) {
        stack.push_back(entry);
      } else {
        result.push_back(entry);
      }
    }
  }
  return result;
}

std::string PathFromUri(const std::string &uri) {
  if (uri.rfind("file://", 0) != 0) {
    return uri;
  }
  GFile *file = g_file_new_for_uri(uri.c_str());
  gchar *path = g_file_get_path(file);
  std::string result = path ? path : uri;
  g_free(path);
  g_object_unref(file);
  return result;
}

std::string UriFromPath(const std::string &path) {
  if (path.rfind("://", 0) != std::string::npos || path.find("://") != std::string::npos) {
    if (path.find("://") != std::string::npos) {
      return path;
    }
  }
  GFile *file = g_file_new_for_path(path.c_str());
  gchar *uri = g_file_get_uri(file);
  std::string result = uri ? uri : path;
  g_free(uri);
  g_object_unref(file);
  return result;
}

std::string ReadFile(const std::string &path) {
  gchar *contents = nullptr;
  gsize length = 0;
  if (!g_file_get_contents(path.c_str(), &contents, &length, nullptr)) {
    return {};
  }
  std::string result(contents, length);
  g_free(contents);
  return result;
}

bool WriteFile(const std::string &path, const std::string &contents) {
  return g_file_set_contents(path.c_str(), contents.data(), static_cast<gssize>(contents.size()), nullptr);
}

bool CopyFile(const std::string &source, const std::string &destination) {
  GFile *src = g_file_new_for_path(source.c_str());
  GFile *dest = g_file_new_for_path(destination.c_str());
  GError *error = nullptr;
  const gboolean ok = g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, &error);
  if (error) {
    g_error_free(error);
  }
  g_object_unref(src);
  g_object_unref(dest);
  return ok == TRUE;
}

bool Remove(const std::string &path) { return g_unlink(path.c_str()) == 0; }

std::string PrettySize(int64_t bytes) {
  if (bytes < 0) {
    return {};
  }
  if (bytes < 1024) {
    return std::to_string(bytes) + " B";
  }
  if (bytes < 1024 * 1024) {
    return std::to_string(bytes / 1024) + " KB";
  }
  if (bytes < 1024LL * 1024 * 1024) {
    char buf[32];
    g_snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    return buf;
  }
  char buf[32];
  g_snprintf(buf, sizeof(buf), "%.1f GB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
  return buf;
}

namespace {

int64_t QueryFilesystemAttribute(const std::string &path, const char *attribute) {
  GFile *file = g_file_new_for_path(path.c_str());
  GError *error = nullptr;
  GFileInfo *info = g_file_query_filesystem_info(file, attribute, nullptr, &error);
  int64_t result = -1;
  if (info) {
    result = static_cast<int64_t>(g_file_info_get_attribute_uint64(info, attribute));
    g_object_unref(info);
  }
  if (error) {
    g_error_free(error);
  }
  g_object_unref(file);
  return result;
}

}  // namespace

int64_t FreeSpaceBytes(const std::string &path) {
  return QueryFilesystemAttribute(path, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
}

int64_t TotalSpaceBytes(const std::string &path) {
  return QueryFilesystemAttribute(path, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
}

}  // namespace FileUtils
