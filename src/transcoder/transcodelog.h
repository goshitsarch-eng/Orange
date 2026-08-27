#ifndef STRAWBERRY_TRANSCODELOG_H
#define STRAWBERRY_TRANSCODELOG_H

#include "utilities/strutils.h"

#include <glib.h>

#include <string>
#include <vector>

namespace TranscodeLog {

inline std::string FormatLine(const std::string &timestamp, const std::string &message) {
  if (timestamp.empty()) {
    return message;
  }
  if (message.empty()) {
    return timestamp;
  }
  return timestamp + ": " + message;
}

inline std::string NowStamp() {
  GDateTime *now = g_date_time_new_now_local();
  if (!now) {
    return {};
  }
  gchar *text = g_date_time_format(now, "%a %b %e %H:%M:%S %Y");
  g_date_time_unref(now);
  std::string result = text ? text : "";
  g_free(text);
  return result;
}

inline void Append(std::vector<std::string> *lines, const std::string &line) {
  if (lines && !line.empty()) {
    lines->push_back(line);
  }
}

inline void Clear(std::vector<std::string> *lines) {
  if (lines) {
    lines->clear();
  }
}

inline std::string Join(const std::vector<std::string> &lines) { return StrUtils::Join(lines, "\n"); }

inline std::string LastLine(const std::vector<std::string> &lines) { return lines.empty() ? std::string() : lines.back(); }

}  // namespace TranscodeLog

#endif
