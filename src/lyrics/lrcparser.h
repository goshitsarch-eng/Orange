#ifndef STRAWBERRY_LRCPARSER_H
#define STRAWBERRY_LRCPARSER_H

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace LrcParser {

struct Line {
  int64_t timestamp_ms = 0;
  std::string text;
};

inline bool LooksSynced(const std::string &text) {
  const size_t open = text.find('[');
  if (open == std::string::npos) {
    return false;
  }
  const size_t close = text.find(']', open);
  if (close == std::string::npos || close <= open + 3) {
    return false;
  }
  return std::isdigit(static_cast<unsigned char>(text[open + 1])) != 0;
}

inline int64_t ParseTimestamp(const std::string &tag) {
  int minutes = 0;
  int seconds = 0;
  int fraction = 0;
  int frac_digits = 0;
  size_t i = 0;
  while (i < tag.size() && std::isdigit(static_cast<unsigned char>(tag[i]))) {
    minutes = minutes * 10 + (tag[i] - '0');
    ++i;
  }
  if (i >= tag.size() || tag[i] != ':') {
    return -1;
  }
  ++i;
  while (i < tag.size() && std::isdigit(static_cast<unsigned char>(tag[i]))) {
    seconds = seconds * 10 + (tag[i] - '0');
    ++i;
  }
  if (i < tag.size() && (tag[i] == '.' || tag[i] == ',')) {
    ++i;
    while (i < tag.size() && std::isdigit(static_cast<unsigned char>(tag[i])) && frac_digits < 3) {
      fraction = fraction * 10 + (tag[i] - '0');
      ++frac_digits;
      ++i;
    }
  }
  while (frac_digits > 0 && frac_digits < 3) {
    fraction *= 10;
    ++frac_digits;
  }
  return static_cast<int64_t>(minutes) * 60000 + static_cast<int64_t>(seconds) * 1000 + fraction;
}

inline std::vector<Line> Parse(const std::string &lrc) {
  std::vector<Line> lines;
  size_t start = 0;
  while (start <= lrc.size()) {
    size_t end = lrc.find('\n', start);
    if (end == std::string::npos) {
      end = lrc.size();
    }
    std::string row = lrc.substr(start, end - start);
    if (!row.empty() && row.back() == '\r') {
      row.pop_back();
    }
    std::vector<int64_t> stamps;
    size_t pos = 0;
    while (pos < row.size() && row[pos] == '[') {
      const size_t close = row.find(']', pos);
      if (close == std::string::npos) {
        break;
      }
      const int64_t stamp = ParseTimestamp(row.substr(pos + 1, close - pos - 1));
      if (stamp < 0) {
        break;
      }
      stamps.push_back(stamp);
      pos = close + 1;
    }
    if (!stamps.empty()) {
      while (pos < row.size() && (row[pos] == ' ' || row[pos] == '\t')) {
        ++pos;
      }
      const std::string text = row.substr(pos);
      for (int64_t stamp : stamps) {
        lines.push_back({stamp, text});
      }
    }
    if (end == lrc.size()) {
      break;
    }
    start = end + 1;
  }
  return lines;
}

inline int ActiveLineIndex(const std::vector<Line> &lines, int64_t position_ms) {
  int active = -1;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (lines[i].timestamp_ms <= position_ms) {
      active = static_cast<int>(i);
    } else {
      break;
    }
  }
  return active;
}

inline std::string PlainText(const std::vector<Line> &lines) {
  std::string text;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) {
      text += '\n';
    }
    text += lines[i].text;
  }
  return text;
}

}  // namespace LrcParser

#endif
