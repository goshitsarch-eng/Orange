#include "utilities/musixmatchprovider.h"

#include "utilities/strutils.h"

#include <glib.h>

namespace MusixmatchProvider {

std::string StringFixup(const std::string &text) {
  std::string replaced = StrUtils::Replace(text, "/", "-");
  replaced = StrUtils::Replace(replaced, "'", "-");
  std::string filtered;
  const char *p = replaced.c_str();
  while (*p) {
    gunichar ch = g_utf8_get_char(p);
    if (g_unichar_isalnum(ch) || ch == '-' || ch == ' ') {
      gchar buf[8] = {};
      const gint len = g_unichar_to_utf8(ch, buf);
      if (len > 0) {
        filtered.append(buf, static_cast<size_t>(len));
      }
    }
    p = g_utf8_next_char(p);
  }
  std::string collapsed;
  bool in_space = false;
  for (char ch : filtered) {
    if (ch == ' ') {
      if (!in_space) {
        collapsed.push_back(' ');
      }
      in_space = true;
    } else {
      collapsed.push_back(ch);
      in_space = false;
    }
  }
  collapsed = StrUtils::Trim(collapsed);
  collapsed = StrUtils::Replace(collapsed, " ", "-");
  std::string dashes;
  bool in_dash = false;
  for (char ch : collapsed) {
    if (ch == '-') {
      if (!in_dash) {
        dashes.push_back('-');
      }
      in_dash = true;
    } else {
      dashes.push_back(ch);
      in_dash = false;
    }
  }
  return StrUtils::ToLower(dashes);
}

}  // namespace MusixmatchProvider
