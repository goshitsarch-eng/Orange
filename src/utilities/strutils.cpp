#include "utilities/strutils.h"

#include "utilities/timeutils.h"

#include <unicode/translit.h>
#include <unicode/unistr.h>

#include <glib.h>

#include <algorithm>
#include <cctype>

namespace StrUtils {

std::string ToLower(const std::string &value) {
  std::string result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

std::string ToUpper(const std::string &value) {
  std::string result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;
}

std::string Trim(const std::string &value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  auto begin = std::find_if(value.begin(), value.end(), not_space);
  auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
  if (begin >= end) {
    return {};
  }
  return std::string(begin, end);
}

std::vector<std::string> Split(const std::string &value, char delimiter) {
  std::vector<std::string> parts;
  std::string current;
  for (char c : value) {
    if (c == delimiter) {
      parts.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  parts.push_back(current);
  return parts;
}

std::string Join(const std::vector<std::string> &parts, const std::string &delimiter) {
  std::string result;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) {
      result += delimiter;
    }
    result += parts[i];
  }
  return result;
}

bool StartsWith(const std::string &value, const std::string &prefix) {
  return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool EndsWith(const std::string &value, const std::string &suffix) {
  return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool ContainsInsensitive(const std::string &haystack, const std::string &needle) {
  return ToLower(haystack).find(ToLower(needle)) != std::string::npos;
}

std::string Replace(const std::string &value, const std::string &from, const std::string &to) {
  if (from.empty()) {
    return value;
  }
  std::string result = value;
  size_t pos = 0;
  while ((pos = result.find(from, pos)) != std::string::npos) {
    result.replace(pos, from.size(), to);
    pos += to.size();
  }
  return result;
}

std::string SqlLikeEscape(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (char c : value) {
    if (c == '%' || c == '_' || c == '\\') {
      result.push_back('\\');
    }
    result.push_back(c);
  }
  return result;
}

std::string SqlQuote(const std::string &value) {
  return "'" + Replace(value, "'", "''") + "'";
}

std::string UriEscape(const std::string &value) {
  gchar *escaped = g_uri_escape_string(value.c_str(), nullptr, TRUE);
  std::string result = escaped ? escaped : value;
  g_free(escaped);
  return result;
}

std::string JsonEscape(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (unsigned char ch : value) {
    switch (ch) {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (ch < 0x20) {
          char buf[8];
          g_snprintf(buf, sizeof(buf), "\\u%04x", ch);
          result += buf;
        } else {
          result.push_back(static_cast<char>(ch));
        }
        break;
    }
  }
  return result;
}

std::string Transliterate(const std::string &value) {
  UErrorCode status = U_ZERO_ERROR;
  icu::Transliterator *transliterator = icu::Transliterator::createInstance("Any-Latin; Latin-ASCII", UTRANS_FORWARD, status);
  if (U_FAILURE(status) || !transliterator) {
    return value;
  }
  icu::UnicodeString unicode = icu::UnicodeString::fromUTF8(value);
  transliterator->transliterate(unicode);
  delete transliterator;
  std::string result;
  unicode.toUTF8String(result);
  return result;
}

namespace {

std::string ReplaceVariable(const std::string &variable, const Song &song, const std::string &newline) {
  if (variable == "%title%") {
    return song.PrettyTitle();
  }
  if (variable == "%titlesort%") {
    return song.titlesort();
  }
  if (variable == "%album%") {
    return song.album();
  }
  if (variable == "%albumsort%") {
    return song.albumsort();
  }
  if (variable == "%artist%") {
    return song.artist();
  }
  if (variable == "%artistsort%") {
    return song.artistsort();
  }
  if (variable == "%albumartist%") {
    return song.EffectiveAlbumartist();
  }
  if (variable == "%albumartistsort%") {
    return song.albumartistsort();
  }
  if (variable == "%track%") {
    return std::to_string(song.track());
  }
  if (variable == "%disc%") {
    return std::to_string(song.disc());
  }
  if (variable == "%year%") {
    return song.year() > 0 ? std::to_string(song.year()) : std::string();
  }
  if (variable == "%originalyear%") {
    return song.originalyear() > 0 ? std::to_string(song.originalyear()) : std::string();
  }
  if (variable == "%genre%") {
    return song.genre();
  }
  if (variable == "%composer%") {
    return song.composer();
  }
  if (variable == "%composersort%") {
    return song.composersort();
  }
  if (variable == "%performer%") {
    return song.performer();
  }
  if (variable == "%performersort%") {
    return song.performersort();
  }
  if (variable == "%grouping%") {
    return song.grouping();
  }
  if (variable == "%length%") {
    return Utilities::PrettyTimeNanosec(song.length_nanosec());
  }
  if (variable == "%filename%") {
    return song.basefilename();
  }
  if (variable == "%url%") {
    return song.url();
  }
  if (variable == "%playcount%") {
    return std::to_string(song.playcount());
  }
  if (variable == "%skipcount%") {
    return std::to_string(song.skipcount());
  }
  if (variable == "%rating%") {
    return song.rating() > 0 ? std::to_string(song.rating()) : std::string();
  }
  if (variable == "%newline%") {
    return newline;
  }
  return variable;
}

}  // namespace

std::string ReplaceMessage(const std::string &message, const Song &song, const std::string &newline) {
  std::string copy;
  copy.reserve(message.size());
  for (size_t i = 0; i < message.size();) {
    if (message[i] == '%') {
      const size_t end = message.find('%', i + 1);
      if (end != std::string::npos) {
        copy += ReplaceVariable(message.substr(i, end - i + 1), song, newline);
        i = end + 1;
        continue;
      }
    }
    copy.push_back(message[i]);
    ++i;
  }
  return copy;
}

}  // namespace StrUtils
