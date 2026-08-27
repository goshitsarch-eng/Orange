#ifndef STRAWBERRY_FILEMANAGERREVEAL_H
#define STRAWBERRY_FILEMANAGERREVEAL_H

#include "utilities/fileutils.h"

#include <cctype>
#include <string>
#include <vector>

namespace FileManagerReveal {

enum class Style { SelectFile, DirectoryOnly, Caja, PassFile, FallbackDirectory };

struct Launch {
  std::string program;
  std::vector<std::string> args;
  std::string directory;
  bool fallback_open_directory = false;
};

struct ParsedExec {
  std::string program;
  std::vector<std::string> args;
};

// Qt filemanagerutils.cpp: remove [%][a-zA-Z]*( |$) from the desktop Exec line.
inline std::string StripDesktopExecFields(const std::string &exec) {
  std::string out;
  out.reserve(exec.size());
  for (size_t i = 0; i < exec.size();) {
    if (exec[i] == '%' && i + 1 < exec.size() && std::isalpha(static_cast<unsigned char>(exec[i + 1]))) {
      i += 2;
      while (i < exec.size() && std::isalpha(static_cast<unsigned char>(exec[i]))) {
        ++i;
      }
      if (i < exec.size() && exec[i] == ' ') {
        ++i;
      }
      continue;
    }
    out.push_back(exec[i]);
    ++i;
  }
  return out;
}

inline std::vector<std::string> SplitCommand(const std::string &cmd) {
  std::vector<std::string> parts;
  std::string cur;
  for (char c : cmd) {
    if (c == ' ' || c == '\t') {
      if (!cur.empty()) {
        parts.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) {
    parts.push_back(cur);
  }
  return parts;
}

// Qt only rewrites an Exec program that starts with /usr/bin/.
inline std::string CommandBasename(const std::string &command) {
  if (command.compare(0, 9, "/usr/bin/") != 0) {
    return command;
  }
  const auto slash = command.find_last_of('/');
  return slash == std::string::npos ? command : command.substr(slash + 1);
}

inline bool StartsWith(const std::string &value, const char *prefix) {
  const size_t n = std::char_traits<char>::length(prefix);
  return value.size() >= n && value.compare(0, n, prefix) == 0;
}

inline Style StyleFor(const std::string &basename) {
  if (basename.empty() || basename == "exo-open") {
    return Style::FallbackDirectory;
  }
  if (StartsWith(basename, "nautilus") || StartsWith(basename, "dolphin") || StartsWith(basename, "konqueror") ||
      StartsWith(basename, "kfmclient")) {
    return Style::SelectFile;
  }
  if (StartsWith(basename, "caja")) {
    return Style::Caja;
  }
  if (StartsWith(basename, "pcmanfm") || StartsWith(basename, "thunar") || StartsWith(basename, "spacefm")) {
    return Style::DirectoryOnly;
  }
  return Style::PassFile;
}

inline ParsedExec ParseExec(const std::string &exec) {
  ParsedExec parsed;
  const std::vector<std::string> parts = SplitCommand(StripDesktopExecFields(exec));
  if (parts.empty()) {
    return parsed;
  }
  parsed.program = CommandBasename(parts.front());
  parsed.args.assign(parts.begin() + 1, parts.end());
  return parsed;
}

inline Launch BuildLaunch(const std::string &program, const std::vector<std::string> &exec_args, const std::string &directory,
                          const std::string &file) {
  Launch launch;
  launch.directory = directory;
  launch.program = program;
  launch.args = exec_args;
  switch (StyleFor(program)) {
    case Style::SelectFile:
      launch.args.push_back("--select");
      launch.args.push_back(file);
      break;
    case Style::Caja:
      launch.args.push_back("--no-desktop");
      launch.args.push_back(directory);
      break;
    case Style::DirectoryOnly:
      launch.args.push_back(directory);
      break;
    case Style::PassFile:
      launch.args.push_back(file);
      break;
    case Style::FallbackDirectory:
      launch.fallback_open_directory = true;
      launch.program.clear();
      launch.args.clear();
      break;
  }
  return launch;
}

inline bool IsLocalUrl(const std::string &url) {
  if (url.empty()) {
    return false;
  }
  if (url.rfind("file://", 0) == 0) {
    return true;
  }
  return url.find("://") == std::string::npos;
}

inline std::string LocalPath(const std::string &url_or_path) {
  if (!IsLocalUrl(url_or_path)) {
    return {};
  }
  if (url_or_path.rfind("file://", 0) == 0) {
    return FileUtils::PathFromUri(url_or_path);
  }
  return url_or_path;
}

// Qt OpenInFileBrowser skips non-local URLs and missing files.
inline std::vector<std::string> LocalExistingPaths(const std::vector<std::string> &urls_or_paths) {
  std::vector<std::string> result;
  for (const std::string &item : urls_or_paths) {
    const std::string path = LocalPath(item);
    if (path.empty() || !FileUtils::Exists(path)) {
      continue;
    }
    result.push_back(path);
  }
  return result;
}

}  // namespace FileManagerReveal

#endif  // STRAWBERRY_FILEMANAGERREVEAL_H
