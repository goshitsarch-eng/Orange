#include "utilities/filemanagerutils.h"

#include "utilities/fileutils.h"

#include <gio/gio.h>

namespace FileManagerUtils {

bool OpenInFileManager(const std::string &path) {
  GFile *file = g_file_new_for_path(path.c_str());
  gchar *uri = g_file_get_uri(file);
  GError *error = nullptr;
  const bool ok = g_app_info_launch_default_for_uri(uri, nullptr, &error);
  if (error) {
    g_error_free(error);
  }
  g_free(uri);
  g_object_unref(file);
  return ok;
}

bool OpenFolder(const std::string &path) { return OpenInFileManager(FileUtils::IsDirectory(path) ? path : FileUtils::DirName(path)); }

}  // namespace FileManagerUtils
