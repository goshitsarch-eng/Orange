#include "playlistparsers/parserbase.h"

#include "playlistparsers/parsercollectionlookup.h"
#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cctype>

namespace {

int g_path_type_override = -1;
CollectionBackend *g_collection_backend = nullptr;

}  // namespace

void ParserBase::SetPathTypeOverride(int type) { g_path_type_override = type; }

void ParserBase::SetCollectionBackend(CollectionBackend *backend) { g_collection_backend = backend; }

bool ParserBase::HasUrlScheme(const std::string &value) {
  const size_t colon = value.find(':');
  if (colon == std::string::npos || colon < 2) {
    return false;
  }
  for (size_t i = 0; i < colon; ++i) {
    if (!std::isalpha(static_cast<unsigned char>(value[i]))) {
      return false;
    }
  }
  return true;
}

Song ParserBase::LoadSong(const std::string &playlist_dir, const std::string &entry) {
  Song song;
  std::string path = StrUtils::Trim(XmlUnescape(entry));
  if (path.empty()) {
    return song;
  }
  if (HasUrlScheme(path)) {
    song.set_source(StrUtils::StartsWith(StrUtils::ToLower(path), "file:") ? Song::Source::LocalFile : Song::Source::Stream);
    song.set_url(path);
    song.set_title(FileUtils::BaseName(FileUtils::PathFromUri(path)));
    song.set_valid(true);
    return ParserCollectionLookup::Resolve(song, g_collection_backend);
  }
  if (!path.empty() && path[0] != '/') {
    path = FileUtils::Join(playlist_dir, path);
  }
  song.set_source(Song::Source::LocalFile);
  song.set_url(FileUtils::UriFromPath(path));
  song.set_title(FileUtils::BaseName(path));
  song.set_basefilename(FileUtils::BaseName(path));
  song.set_valid(true);
  return ParserCollectionLookup::Resolve(song, g_collection_backend);
}

std::string ParserBase::URLOrFilename(const std::string &url, const std::string &playlist_dir) {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  const int stored = g_path_type_override >= 0
                         ? g_path_type_override
                         : settings.IntValue(PlaylistSettings::kPathType, static_cast<int>(PlaylistSettings::kDefaultPathType));
  const auto type = static_cast<PlaylistSettings::PathType>(stored);
  if (url.rfind("file://", 0) == 0) {
    std::string path = FileUtils::PathFromUri(url);
    if (type == PlaylistSettings::PathType::Absolute) {
      return path;
    }
    if (!playlist_dir.empty() && StrUtils::StartsWith(path, playlist_dir + "/")) {
      return path.substr(playlist_dir.size() + 1);
    }
    if (type == PlaylistSettings::PathType::Relative) {
      return FileUtils::BaseName(path);
    }
    return path;
  }
  return url;
}

std::string ParserBase::XmlEscape(const std::string &value) {
  std::string result;
  result.reserve(value.size());
  for (char c : value) {
    switch (c) {
      case '&':
        result += "&amp;";
        break;
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '"':
        result += "&quot;";
        break;
      case '\'':
        result += "&apos;";
        break;
      default:
        result.push_back(c);
        break;
    }
  }
  return result;
}

std::string ParserBase::XmlUnescape(const std::string &value) {
  std::string result = StrUtils::Replace(value, "&amp;", "&");
  result = StrUtils::Replace(result, "&lt;", "<");
  result = StrUtils::Replace(result, "&gt;", ">");
  result = StrUtils::Replace(result, "&quot;", "\"");
  result = StrUtils::Replace(result, "&apos;", "'");
  return result;
}

std::string ParserBase::TagText(const std::string &data, const std::string &tag, size_t from, size_t *next) {
  const std::string open = "<" + tag;
  const std::string close = "</" + tag + ">";
  const std::string lower = StrUtils::ToLower(data);
  const std::string open_l = StrUtils::ToLower(open);
  const std::string close_l = StrUtils::ToLower(close);
  size_t start = lower.find(open_l, from);
  if (start == std::string::npos) {
    if (next) {
      *next = std::string::npos;
    }
    return {};
  }
  const size_t gt = data.find('>', start);
  if (gt == std::string::npos) {
    return {};
  }
  const size_t end = lower.find(close_l, gt + 1);
  if (end == std::string::npos) {
    return {};
  }
  if (next) {
    *next = end + close.size();
  }
  return XmlUnescape(data.substr(gt + 1, end - (gt + 1)));
}

std::string ParserBase::AttributeValue(const std::string &element, const std::string &name) {
  const std::string lower = StrUtils::ToLower(element);
  const std::string key = StrUtils::ToLower(name) + "=";
  size_t pos = lower.find(key);
  if (pos == std::string::npos) {
    return {};
  }
  pos += key.size();
  if (pos >= element.size()) {
    return {};
  }
  const char quote = element[pos];
  if (quote != '"' && quote != '\'') {
    const size_t end = element.find_first_of(" \t>", pos);
    return XmlUnescape(element.substr(pos, end == std::string::npos ? std::string::npos : end - pos));
  }
  const size_t end = element.find(quote, pos + 1);
  if (end == std::string::npos) {
    return {};
  }
  return XmlUnescape(element.substr(pos + 1, end - pos - 1));
}
