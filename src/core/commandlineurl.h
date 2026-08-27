#ifndef STRAWBERRY_COMMANDLINEURL_H
#define STRAWBERRY_COMMANDLINEURL_H

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <string>

namespace CommandlineUrl {

inline std::string Scheme(const std::string &value) {
  const std::string::size_type sep = value.find("://");
  if (sep == std::string::npos || sep == 0) {
    return {};
  }
  return StrUtils::ToLower(value.substr(0, sep));
}

inline bool HasScheme(const std::string &value) { return !Scheme(value).empty(); }

// Qt QUrl::isLocalFile: file: scheme or a bare filesystem path.
inline bool IsLocalFile(const std::string &value) {
  if (value.empty()) {
    return false;
  }
  const std::string scheme = Scheme(value);
  return scheme.empty() || scheme == "file";
}

inline std::string LocalPath(const std::string &value) {
  if (!IsLocalFile(value)) {
    return {};
  }
  return FileUtils::PathFromUri(value);
}

inline bool LocalFileExists(const std::string &value) {
  const std::string path = LocalPath(value);
  return !path.empty() && FileUtils::Exists(path);
}

inline std::string AbsoluteFileUrl(const std::string &value) {
  const std::string path = LocalPath(value);
  if (path.empty()) {
    return value;
  }
  return FileUtils::UriFromPath(FileUtils::CanonicalPath(path));
}

// Qt QUrl::fromUserInput for leftover CLI tokens that are not existing files.
inline std::string FromUserInput(const std::string &value) {
  if (value.empty()) {
    return {};
  }
  if (HasScheme(value)) {
    return value;
  }
  if (value[0] == '/' || value.find('/') != std::string::npos || value.find('\\') != std::string::npos) {
    return FileUtils::UriFromPath(FileUtils::CanonicalPath(value));
  }
  return "http://" + value;
}

// Qt CommandlineOptions leftover args: existing files become absolute file URLs.
inline std::string FromArg(const std::string &value) {
  if (LocalFileExists(value)) {
    return AbsoluteFileUrl(value);
  }
  return FromUserInput(value);
}

}  // namespace CommandlineUrl

#endif
