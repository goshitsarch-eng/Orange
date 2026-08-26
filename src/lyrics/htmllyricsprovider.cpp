#include "lyrics/htmllyricsprovider.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <cctype>

namespace {

std::string TagName(std::string pattern) {
  pattern = StrUtils::Replace(pattern, "\\/", "/");
  pattern = StrUtils::Replace(pattern, "[^>]*", "");
  pattern = StrUtils::Replace(pattern, "[^>]+", "");
  if (!pattern.empty() && pattern.front() == '<') {
    pattern.erase(pattern.begin());
  }
  if (!pattern.empty() && pattern.back() == '>') {
    pattern.pop_back();
  }
  if (StrUtils::StartsWith(pattern, "/")) {
    pattern.erase(0, 1);
  }
  const auto space = pattern.find_first_of(" \t");
  if (space != std::string::npos) {
    pattern = pattern.substr(0, space);
  }
  return StrUtils::ToLower(pattern);
}

size_t FindInsensitive(const std::string &haystack, const std::string &needle, size_t from) {
  if (needle.empty() || from >= haystack.size()) {
    return std::string::npos;
  }
  const std::string lower_needle = StrUtils::ToLower(needle);
  for (size_t i = from; i + lower_needle.size() <= haystack.size(); ++i) {
    bool match = true;
    for (size_t j = 0; j < lower_needle.size(); ++j) {
      if (std::tolower(static_cast<unsigned char>(haystack[i + j])) != static_cast<unsigned char>(lower_needle[j])) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return std::string::npos;
}

size_t FindOpenTag(const std::string &content, const std::string &tag, size_t from, size_t *after) {
  const std::string needle = "<" + tag;
  size_t pos = from;
  while (pos < content.size()) {
    pos = FindInsensitive(content, needle, pos);
    if (pos == std::string::npos) {
      return std::string::npos;
    }
    if (pos + needle.size() < content.size()) {
      const unsigned char next = static_cast<unsigned char>(content[pos + needle.size()]);
      if (std::isalnum(next) || next == '-') {
        pos += needle.size();
        continue;
      }
    }
    const size_t gt = content.find('>', pos);
    if (gt == std::string::npos) {
      return std::string::npos;
    }
    if (after) {
      *after = gt + 1;
    }
    return pos;
  }
  return std::string::npos;
}

size_t FindCloseTag(const std::string &content, const std::string &tag, size_t from, size_t *after) {
  const std::string needle = "</" + tag + ">";
  const size_t pos = FindInsensitive(content, needle, from);
  if (pos == std::string::npos) {
    return std::string::npos;
  }
  if (after) {
    *after = pos + needle.size();
  }
  return pos;
}

std::string Collapse(const std::string &text, char ch) {
  std::string result;
  result.reserve(text.size());
  bool last = false;
  for (char c : text) {
    if (c == ch) {
      if (!last) {
        result.push_back(c);
      }
      last = true;
    } else {
      result.push_back(c);
      last = false;
    }
  }
  return result;
}

}  // namespace

HtmlLyricsProvider::HtmlLyricsProvider(std::string name, std::string start_tag, std::string end_tag, std::string lyrics_start, bool multiple)
    : name_(std::move(name)), start_tag_(std::move(start_tag)), end_tag_(std::move(end_tag)), lyrics_start_(std::move(lyrics_start)), multiple_(multiple) {}

std::string HtmlLyricsProvider::ParseLyricsFromHTML(const std::string &content, const std::string &start_tag, const std::string &end_tag,
                                                    const std::string &lyrics_start, bool multiple) {
  if (content.empty() || lyrics_start.empty()) {
    return {};
  }
  const std::string open_name = TagName(start_tag);
  const std::string close_name = TagName(end_tag.empty() ? start_tag : end_tag);
  std::string lyrics;
  size_t search_from = 0;
  do {
    size_t marker = content.find(lyrics_start, search_from);
    if (marker == std::string::npos) {
      break;
    }
    size_t start_lyrics = marker + lyrics_start.size();
    if (!lyrics_start.empty() && lyrics_start.back() != '>') {
      const size_t gt = content.find('>', start_lyrics);
      if (gt != std::string::npos && gt - start_lyrics < 200) {
        start_lyrics = gt + 1;
      }
    }
    size_t idx = start_lyrics;
    int tags = 1;
    size_t end_lyrics = std::string::npos;
    while (tags > 0 && idx < content.size()) {
      size_t after_open = 0;
      size_t after_close = 0;
      const size_t open_pos = FindOpenTag(content, open_name, idx, &after_open);
      const size_t close_pos = FindCloseTag(content, close_name, idx, &after_close);
      if (open_pos != std::string::npos && (close_pos == std::string::npos || open_pos <= close_pos)) {
        ++tags;
        idx = after_open;
      } else if (close_pos != std::string::npos) {
        --tags;
        idx = after_close;
        if (tags == 0) {
          end_lyrics = close_pos;
          search_from = after_close;
        }
      } else {
        break;
      }
    }
    if (end_lyrics != std::string::npos && start_lyrics < end_lyrics) {
      if (!lyrics.empty()) {
        lyrics.push_back('\n');
      }
      lyrics += content.substr(start_lyrics, end_lyrics - start_lyrics);
    } else {
      break;
    }
  } while (multiple && search_from > 0);

  lyrics = JsonUtils::StripHtml(lyrics);
  if (lyrics.size() > 6000 || StrUtils::ContainsInsensitive(lyrics, "there are no lyrics to") ||
      StrUtils::ContainsInsensitive(lyrics, "we do not have the lyrics for")) {
    return {};
  }
  return lyrics;
}

std::string HtmlLyricsProvider::SlugAzLyrics(const std::string &text) {
  std::string slug;
  for (unsigned char ch : StrUtils::ToLower(StrUtils::Transliterate(text))) {
    if (std::isalnum(ch) || ch == '-') {
      slug.push_back(static_cast<char>(ch));
    }
  }
  return slug;
}

std::string HtmlLyricsProvider::SlugDashed(const std::string &text) {
  std::string slug = StrUtils::Replace(text, "/", "-");
  slug = StrUtils::Replace(slug, "'", "-");
  std::string cleaned;
  for (unsigned char ch : slug) {
    if (std::isalnum(ch) || ch == '-' || ch == ' ') {
      cleaned.push_back(static_cast<char>(ch));
    }
  }
  cleaned = StrUtils::Replace(StrUtils::Trim(Collapse(cleaned, ' ')), " ", "-");
  return StrUtils::ToLower(Collapse(cleaned, '-'));
}

std::string HtmlLyricsProvider::SlugElyrics(const std::string &text) {
  std::string slug;
  for (unsigned char ch : StrUtils::Transliterate(text)) {
    if (std::isalnum(ch) || ch == '_' || ch == ',' || ch == '&' || ch == '-' || ch == '(' || ch == ')' || ch == ' ') {
      slug.push_back(static_cast<char>(ch));
    } else {
      slug.push_back('_');
    }
  }
  slug = StrUtils::Replace(StrUtils::Trim(Collapse(slug, ' ')), " ", "-");
  return StrUtils::ToLower(slug);
}

std::string HtmlLyricsProvider::SlugLetras(const std::string &text) {
  std::string slug;
  for (unsigned char ch : StrUtils::Transliterate(text)) {
    if (std::isalnum(ch) || ch == ' ' || ch == '-') {
      slug.push_back(static_cast<char>(ch));
    }
  }
  slug = StrUtils::Replace(StrUtils::Trim(Collapse(slug, ' ')), " ", "-");
  return StrUtils::ToLower(slug);
}

void HtmlLyricsProvider::Fetch(const Song &song, NetworkAccessManager *network, Callback callback) {
  if (!network || song.artist().empty() || song.title().empty()) {
    callback({}, "No artist or title");
    return;
  }
  const std::string url = UrlFor(song);
  if (url.empty()) {
    callback({}, "No lyrics URL");
    return;
  }
  network->Get(url, [this, callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({}, response.error.empty() ? "Lyrics request failed" : response.error);
      return;
    }
    const std::string lyrics = ParseLyricsFromHTML(response.body, start_tag_, end_tag_, lyrics_start_, multiple_);
    if (lyrics.empty()) {
      callback({}, "No lyrics in provider response");
      return;
    }
    callback(lyrics, {});
  }, RequestHeaders());
}
