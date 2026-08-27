#include "utilities/filemanagerutils.h"

#include "utilities/filemanagerreveal.h"
#include "utilities/fileutils.h"

#include <gio/gio.h>
#include <glib.h>

#include <string>
#include <vector>

namespace FileManagerUtils {
namespace {

std::string TrimDesktopId(std::string value) {
  while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
    value.pop_back();
  }
  return value;
}

std::string QueryDefaultDirectoryDesktop() {
  gchar *out = nullptr;
  gchar *err = nullptr;
  gint status = 0;
  GError *error = nullptr;
  gchar *argv[] = {const_cast<gchar *>("xdg-mime"), const_cast<gchar *>("query"), const_cast<gchar *>("default"),
                   const_cast<gchar *>("inode/directory"), nullptr};
  if (!g_spawn_sync(nullptr, argv, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, &out, &err, &status, &error)) {
    if (error) {
      g_error_free(error);
    }
    g_free(out);
    g_free(err);
    return {};
  }
  g_free(err);
  std::string desktop = TrimDesktopId(out ? out : "");
  g_free(out);
  return desktop;
}

std::string FindDesktopExec(const std::string &desktop_file) {
  if (desktop_file.empty()) {
    return {};
  }
  std::vector<std::string> roots;
  if (const gchar *user = g_get_user_data_dir()) {
    roots.push_back(std::string(user) + "/applications");
  }
  const gchar *const *dirs = g_get_system_data_dirs();
  for (int i = 0; dirs && dirs[i]; ++i) {
    roots.push_back(std::string(dirs[i]) + "/applications");
  }
  for (const std::string &root : roots) {
    const std::string path = root + "/" + desktop_file;
    if (!g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
      continue;
    }
    GKeyFile *key = g_key_file_new();
    if (!g_key_file_load_from_file(key, path.c_str(), G_KEY_FILE_NONE, nullptr)) {
      g_key_file_free(key);
      continue;
    }
    gchar *exec = g_key_file_get_string(key, "Desktop Entry", "Exec", nullptr);
    std::string result = exec ? exec : "";
    g_free(exec);
    g_key_file_free(key);
    if (!result.empty()) {
      return result;
    }
  }
  return {};
}

bool OpenDirectoryFallback(const std::string &directory) {
  if (directory.empty()) {
    return false;
  }
  GFile *file = g_file_new_for_path(directory.c_str());
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

bool SpawnLaunch(const FileManagerReveal::Launch &launch) {
  if (launch.fallback_open_directory) {
    return OpenDirectoryFallback(launch.directory);
  }
  if (launch.program.empty()) {
    return OpenDirectoryFallback(launch.directory);
  }
  std::vector<char *> argv;
  argv.push_back(const_cast<char *>(launch.program.c_str()));
  for (const std::string &arg : launch.args) {
    argv.push_back(const_cast<char *>(arg.c_str()));
  }
  argv.push_back(nullptr);
  GError *error = nullptr;
  const gboolean ok = g_spawn_async(nullptr, argv.data(), nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, &error);
  if (error) {
    g_error_free(error);
  }
  return ok == TRUE;
}

}  // namespace

bool OpenInFileManager(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  const std::string directory = FileUtils::IsDirectory(path) ? path : FileUtils::DirName(path);
  return OpenInFileManager(directory, path);
}

bool OpenInFileManager(const std::string &directory, const std::string &file) {
  const std::string dir = directory.empty() ? FileUtils::DirName(file) : directory;
  const std::string target = file.empty() ? dir : file;
  if (dir.empty() && target.empty()) {
    return false;
  }
  const FileManagerReveal::ParsedExec parsed = FileManagerReveal::ParseExec(FindDesktopExec(QueryDefaultDirectoryDesktop()));
  return SpawnLaunch(FileManagerReveal::BuildLaunch(parsed.program, parsed.args, dir, target));
}

bool OpenFolder(const std::string &path) { return OpenInFileManager(FileUtils::IsDirectory(path) ? path : FileUtils::DirName(path)); }

}  // namespace FileManagerUtils
