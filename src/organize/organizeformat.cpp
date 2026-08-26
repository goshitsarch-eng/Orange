#include "organize/organizeformat.h"

#include "constants/timeconstants.h"
#include "organize/organizefilename.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <cctype>
#include <cstdio>
#include <utility>

const char *OrganizeFormat::kKnownTags[] = {"%albumartist", "%artistinitial", "%originalyear", "%samplerate", "%extension",
                                            "%bitdepth",    "%composer",      "%performer",    "%grouping",   "%comment",
                                            "%bitrate",     "%artist",        "%album",        "%title",      "%genre",
                                            "%lyrics",      "%length",        "%year",         "%disc",       "%track",
                                            nullptr};

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

std::string TrimCopy(const std::string &value) {
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }
  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(start, end - start);
}

bool StartsWithThe(const std::string &value) {
  if (value.size() < 4) {
    return false;
  }
  return (value[0] == 't' || value[0] == 'T') && (value[1] == 'h' || value[1] == 'H') && (value[2] == 'e' || value[2] == 'E') &&
         std::isspace(static_cast<unsigned char>(value[3]));
}

}  // namespace

std::string OrganizeFormat::ArtistInitial(const std::string &albumartist) {
  std::string value = TrimCopy(albumartist);
  if (StartsWithThe(value)) {
    value = TrimCopy(value.substr(4));
  }
  if (value.empty()) {
    return {};
  }
  return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(value[0]))));
}

std::string OrganizeFormat::TokenValue(const std::string &token, const Song &song) {
  if (token == "%albumartist") {
    return song.compilation() ? "Various Artists" : song.EffectiveAlbumartist();
  }
  if (token == "%artist") return song.artist();
  if (token == "%album") return song.album();
  if (token == "%title") return song.title();
  if (token == "%genre") return song.genre();
  if (token == "%composer") return song.composer();
  if (token == "%performer") return song.performer();
  if (token == "%grouping") return song.grouping();
  if (token == "%comment") return song.comment();
  if (token == "%lyrics") return song.lyrics();
  if (token == "%year") return song.year() > 0 ? std::to_string(song.year()) : "";
  if (token == "%originalyear") return song.originalyear() > 0 ? std::to_string(song.originalyear()) : "";
  if (token == "%disc") return song.disc() > 0 ? std::to_string(song.disc()) : "";
  if (token == "%bitrate") return song.bitrate() > 0 ? std::to_string(song.bitrate()) : "";
  if (token == "%samplerate") return song.samplerate() > 0 ? std::to_string(song.samplerate()) : "";
  if (token == "%bitdepth") return song.bitdepth() > 0 ? std::to_string(song.bitdepth()) : "";
  if (token == "%length") {
    if (song.length_nanosec() <= 0) {
      return {};
    }
    return std::to_string(song.length_nanosec() / TimeConstants::kNsecPerSec);
  }
  if (token == "%extension") {
    return FileUtils::Extension(FileUtils::PathFromUri(song.url()));
  }
  if (token == "%artistinitial") {
    return ArtistInitial(song.EffectiveAlbumartist());
  }
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

bool OrganizeFormat::TokenHasValue(const std::string &token, const Song &song) { return !TokenValue(token, song).empty(); }

bool OrganizeFormat::IsValid() const {
  int depth = 0;
  for (char ch : format_) {
    if (ch == '{') {
      ++depth;
    } else if (ch == '}') {
      --depth;
      if (depth < 0) {
        return false;
      }
    }
  }
  return depth == 0 && !format_.empty();
}

std::string OrganizeFormat::ExpandTokens(const std::string &pattern, const Song &song) const {
  std::string result = pattern;
  for (int i = 0; kKnownTags[i]; ++i) {
    std::string value = Safe(TokenValue(kKnownTags[i], song));
    if (remove_problematic_) {
      value = OrganizeFilename::RemoveDots(value);
    }
    result = StrUtils::Replace(result, kKnownTags[i], value);
  }
  return result;
}

std::string OrganizeFormat::ApplyFilenameFixes(std::string path, const Song &song) const {
  if (path.empty()) {
    path = song.basefilename();
  }
  if (FileUtils::Extension(path).empty()) {
    const std::string ext = FileUtils::Extension(FileUtils::PathFromUri(song.url()));
    if (!ext.empty()) {
      path += "." + ext;
    }
  }
  return OrganizeFilename::Apply(path, FilenameOptions());
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
  return ApplyFilenameFixes(ExpandTokens(result, song), song);
}
