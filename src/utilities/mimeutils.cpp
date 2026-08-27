#include "utilities/mimeutils.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <gio/gio.h>

namespace MimeUtils {

std::string MimeTypeFromData(const std::string &data) {
  gboolean uncertain = FALSE;
  gchar *type = g_content_type_guess(nullptr, reinterpret_cast<const guchar *>(data.data()), data.size(), &uncertain);
  std::string result = type ? type : "application/octet-stream";
  g_free(type);
  return result;
}

std::string MimeTypeFromPath(const std::string &path) {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  if (ext == "mp3") return "audio/mpeg";
  if (ext == "flac") return "audio/flac";
  if (ext == "ogg" || ext == "oga") return "audio/ogg";
  if (ext == "opus") return "audio/opus";
  if (ext == "m4a" || ext == "mp4") return "audio/mp4";
  if (ext == "wav") return "audio/wav";
  gboolean uncertain = FALSE;
  gchar *type = g_content_type_guess(path.c_str(), nullptr, 0, &uncertain);
  std::string result = type ? type : "application/octet-stream";
  g_free(type);
  return result;
}

}  // namespace MimeUtils
