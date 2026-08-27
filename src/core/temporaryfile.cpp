#include "core/temporaryfile.h"

#include "utilities/fileutils.h"

#include <glib.h>
#include <unistd.h>

TemporaryFile::TemporaryFile(const std::string &filename_pattern) {
  gchar *name = nullptr;
  GError *error = nullptr;
  const gint fd = g_file_open_tmp(filename_pattern.c_str(), &name, &error);
  if (fd >= 0) {
    close(fd);
    filename_ = name ? name : "";
  }
  if (error) {
    g_error_free(error);
  }
  g_free(name);
}

TemporaryFile::~TemporaryFile() {
  if (!filename_.empty()) {
    FileUtils::Remove(filename_);
  }
}
