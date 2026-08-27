#include "utilities/xmlutils.h"

#include "utilities/strutils.h"

#include <glib.h>

namespace XmlUtils {

std::string Escape(const std::string &value) {
  gchar *escaped = g_markup_escape_text(value.c_str(), static_cast<gssize>(value.size()));
  std::string out = escaped ? escaped : value;
  g_free(escaped);
  return out;
}

std::string Unescape(const std::string &value) {
  std::string out = value;
  out = StrUtils::Replace(out, "&lt;", "<");
  out = StrUtils::Replace(out, "&gt;", ">");
  out = StrUtils::Replace(out, "&quot;", "\"");
  out = StrUtils::Replace(out, "&apos;", "'");
  out = StrUtils::Replace(out, "&amp;", "&");
  return out;
}

std::string Tag(const std::string &name, const std::string &value) { return "<" + name + ">" + Escape(value) + "</" + name + ">"; }

}  // namespace XmlUtils
