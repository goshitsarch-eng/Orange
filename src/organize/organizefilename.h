#ifndef STRAWBERRY_ORGANIZEFILENAME_H
#define STRAWBERRY_ORGANIZEFILENAME_H

#include "constants/filenameconstants.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <cctype>
#include <string>
#include <vector>

namespace OrganizeFilename {

struct Options {
  bool remove_problematic = false;
  bool remove_non_fat = false;
  bool remove_non_ascii = false;
  bool allow_ascii_ext = false;
  bool replace_spaces = false;
};

inline bool IsProblematic(char c) {
  for (const char *p = FilenameConstants::kProblematicCharacters; *p; ++p) {
    if (*p == c) {
      return true;
    }
  }
  return false;
}

inline bool IsFatAllowed(unsigned char c) {
  if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
    return true;
  }
  for (const char *p = FilenameConstants::kFatAllowedPunctuation; *p; ++p) {
    if (static_cast<unsigned char>(*p) == c) {
      return true;
    }
  }
  return false;
}

inline bool ShouldTransliterate(const Options &options) {
  return options.remove_non_fat || (options.remove_non_ascii && !options.allow_ascii_ext);
}

inline std::string RemoveProblematic(const std::string &path) {
  std::string result;
  result.reserve(path.size());
  for (char c : path) {
    if (!IsProblematic(c)) {
      result.push_back(c);
    }
  }
  return result;
}

inline std::string RemoveDots(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (char c : value) {
    if (c != '.') {
      result.push_back(c);
    }
  }
  return result;
}

inline std::string RemoveNonFat(const std::string &path) {
  std::string result;
  result.reserve(path.size());
  for (unsigned char c : path) {
    if (IsFatAllowed(c)) {
      result.push_back(static_cast<char>(c));
    }
  }
  return result;
}

inline void AppendUtf8(std::string *out, gunichar ch) {
  char buf[6];
  const int n = g_unichar_to_utf8(ch, buf);
  if (n > 0) {
    out->append(buf, static_cast<size_t>(n));
  }
}

inline std::string RemoveNonAscii(const std::string &path, bool allow_extended) {
  const gunichar limit = allow_extended ? 255 : 128;
  std::string result;
  result.reserve(path.size());
  const char *p = path.c_str();
  const char *end = p + path.size();
  while (p < end) {
    const gunichar ch = g_utf8_get_char(p);
    p = g_utf8_next_char(p);
    if (ch < limit) {
      AppendUtf8(&result, ch);
      continue;
    }
    gunichar decomp[18];
    const gsize n = g_unichar_fully_decompose(ch, FALSE, decomp, 18);
    if (n > 0 && decomp[0] < limit) {
      AppendUtf8(&result, decomp[0]);
    }
  }
  return result;
}

inline std::string CollapseWhitespace(const std::string &path) {
  std::string result;
  result.reserve(path.size());
  bool pending_space = false;
  for (unsigned char c : path) {
    if (std::isspace(c)) {
      pending_space = true;
      continue;
    }
    if (pending_space && !result.empty()) {
      result.push_back(' ');
    }
    pending_space = false;
    result.push_back(static_cast<char>(c));
  }
  return result;
}

inline std::string StripInvalidPrefixes(const std::string &path) {
  const auto parts = StrUtils::Split(path, '/');
  std::vector<std::string> cleaned;
  cleaned.reserve(parts.size());
  for (std::string part : parts) {
    // Strip every leading dot, not just one: "...album" left ".." behind, which walks out of the destination
    // directory once the relative path is joined onto it.
    while (!part.empty() && part[0] == FilenameConstants::kInvalidPrefixCharacters[0]) {
      part.erase(0, 1);
    }
    part = StrUtils::Trim(part);
    cleaned.push_back(part);
  }
  return StrUtils::Join(cleaned, "/");
}

inline std::string ReplaceSpaces(const std::string &path) {
  std::string result = path;
  for (char &c : result) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      c = '_';
    }
  }
  return result;
}

inline std::string Apply(std::string path, const Options &options) {
  if (options.remove_problematic) {
    path = RemoveProblematic(path);
  }
  if (ShouldTransliterate(options)) {
    path = StrUtils::Transliterate(path);
  }
  if (options.remove_non_fat) {
    path = RemoveNonFat(path);
  }
  if (options.remove_non_ascii) {
    path = RemoveNonAscii(path, options.allow_ascii_ext);
  }
  path = CollapseWhitespace(path);
  path = StripInvalidPrefixes(path);
  if (options.replace_spaces) {
    path = ReplaceSpaces(path);
  }
  return path;
}

}  // namespace OrganizeFilename

#endif
