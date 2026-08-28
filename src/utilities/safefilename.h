#ifndef STRAWBERRY_SAFEFILENAME_H
#define STRAWBERRY_SAFEFILENAME_H

#include <string>

// Builds a single filename component out of text this application does not control: tags read from files,
// identifiers returned by a streaming server, names typed into a dialog.  Without this a value containing
// "/" or ".." reaches a path join and the file is written outside the directory it was meant for.
namespace SafeFilename {

inline bool IsDotsOnly(const std::string &value) {
  if (value.empty()) {
    return false;
  }
  for (char c : value) {
    if (c != '.') {
      return false;
    }
  }
  return true;
}

inline std::string Component(const std::string &value, const std::string &fallback = "unknown") {
  std::string out;
  out.reserve(value.size());
  for (unsigned char c : value) {
    // Separators and the Windows drive separator would split the component in two; control characters and
    // NUL truncate or corrupt the path.
    if (c == '/' || c == '\\' || c == ':' || c < 0x20 || c == 0x7f) {
      out += '_';
    }
    else {
      out += static_cast<char>(c);
    }
  }
  // A leading dot hides the file; a component of nothing but dots is "." or "..", which walks the tree.
  std::string::size_type start = 0;
  while (start < out.size() && (out[start] == '.' || out[start] == ' ')) {
    ++start;
  }
  out.erase(0, start);
  while (!out.empty() && (out.back() == ' ' || out.back() == '.')) {
    out.pop_back();
  }
  if (out.empty() || IsDotsOnly(out)) {
    return fallback;
  }
  return out;
}

}  // namespace SafeFilename

#endif
