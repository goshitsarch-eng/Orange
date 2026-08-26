#include "organize/organize.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <cctype>
#include <cstdio>
#include <utility>

OrganizeFormat::OrganizeFormat(std::string format) : format_(std::move(format)) {}

namespace {

std::string Safe(const std::string &value) {
  std::string result = value;
  for (char &c : result) {
    if (c == '/' || c == '\\' || c == ':') {
      c = '_';
    }
  }
  return result;
}

std::string TokenValue(const std::string &token, const Song &song) {
  if (token == "%albumartist") return song.EffectiveAlbumartist();
  if (token == "%artist") return song.artist();
  if (token == "%album") return song.album();
  if (token == "%title") return song.title();
  if (token == "%genre") return song.genre();
  if (token == "%composer") return song.composer();
  if (token == "%performer") return song.performer();
  if (token == "%grouping") return song.grouping();
  if (token == "%comment") return song.comment();
  if (token == "%year") return song.year() > 0 ? std::to_string(song.year()) : "";
  if (token == "%originalyear") return song.originalyear() > 0 ? std::to_string(song.originalyear()) : "";
  if (token == "%disc") return song.disc() > 0 ? std::to_string(song.disc()) : "";
  if (token == "%track") {
    if (song.track() <= 0) {
      return {};
    }
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d", song.track());
    return buf;
  }
  return {};
}

}  // namespace

bool OrganizeFormat::TokenHasValue(const std::string &token, const Song &song) { return !TokenValue(token, song).empty(); }

std::string OrganizeFormat::ExpandTokens(const std::string &pattern, const Song &song) const {
  std::string result = pattern;
  static const char *tokens[] = {"%albumartist", "%artist", "%album",    "%title", "%genre", "%composer", "%performer",
                                 "%grouping",    "%comment", "%originalyear", "%year",  "%disc", "%track",   nullptr};
  for (int i = 0; tokens[i]; ++i) {
    result = StrUtils::Replace(result, tokens[i], Safe(TokenValue(tokens[i], song)));
  }
  return result;
}

std::string OrganizeFormat::GetFilenameForSong(const Song &song) const {
  std::string result;
  result.reserve(format_.size());
  for (size_t i = 0; i < format_.size();) {
    if (format_[i] == '{') {
      const size_t end = format_.find('}', i);
      if (end == std::string::npos) {
        result += format_[i++];
        continue;
      }
      const std::string inner = format_.substr(i + 1, end - i - 1);
      bool keep = true;
      size_t pos = 0;
      while ((pos = inner.find('%', pos)) != std::string::npos) {
        size_t len = 1;
        while (pos + len < inner.size() && std::isalpha(static_cast<unsigned char>(inner[pos + len]))) {
          ++len;
        }
        if (!TokenHasValue(inner.substr(pos, len), song)) {
          keep = false;
          break;
        }
        pos += len;
      }
      if (keep) {
        result += ExpandTokens(inner, song);
      }
      i = end + 1;
      continue;
    }
    result += format_[i++];
  }
  return ExpandTokens(result, song);
}

std::vector<Organize::Error> Organize::Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move) {
  std::vector<Error> errors;
  if (destination.empty()) {
    errors.push_back({"", "Destination folder is empty"});
    return errors;
  }
  for (const Song &song : songs) {
    const std::string src = FileUtils::PathFromUri(song.url());
    const std::string relative = format.GetFilenameForSong(song);
    if (relative.empty()) {
      errors.push_back({song.PrettyTitleWithArtist(), "Filename format produced an empty path"});
      continue;
    }
    const std::string dest = FileUtils::Join(destination, relative);
    if (src.empty() || !FileUtils::Exists(src)) {
      errors.push_back({song.PrettyTitleWithArtist(), "Source file is missing"});
      continue;
    }
    g_mkdir_with_parents(FileUtils::DirName(dest).c_str(), 0755);
    bool ok = false;
    if (move) {
      ok = g_rename(src.c_str(), dest.c_str()) == 0;
      if (!ok) {
        ok = FileUtils::CopyFile(src, dest) && FileUtils::Remove(src);
      }
    } else {
      ok = FileUtils::CopyFile(src, dest);
    }
    if (!ok) {
      errors.push_back({song.PrettyTitleWithArtist(), "Could not write " + dest});
    }
  }
  return errors;
}
