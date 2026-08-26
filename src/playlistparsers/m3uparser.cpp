#include "playlistparsers/m3uparser.h"

#include "core/logging.h"
#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <sstream>

const int M3UParser::kMaxNestingDepth = 5;

bool M3UParser::TryMagic(const std::string &data) const {
  return StrUtils::ContainsInsensitive(data, "#extm3u") || StrUtils::ContainsInsensitive(data, "#extinf");
}

bool M3UParser::IsNestedPlaylistReference(const std::string &line) {
  if (HasUrlScheme(line)) {
    return false;
  }
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(line));
  return ext == "m3u" || ext == "m3u8";
}

bool M3UParser::ParseMetadata(const std::string &line, Metadata *metadata) {
  if (!metadata || !StrUtils::StartsWith(line, "#EXTINF:")) {
    return false;
  }
  const std::string info = line.substr(8);
  const size_t comma = info.find(',');
  if (comma == std::string::npos) {
    return false;
  }
  try {
    const long seconds = std::stol(info.substr(0, comma));
    metadata->length_nanosec = seconds > 0 ? seconds * 1000000000LL : -1;
  } catch (...) {
    return false;
  }
  const std::string track_info = info.substr(comma + 1);
  const size_t dash = track_info.find(" - ");
  if (dash == std::string::npos) {
    metadata->title = StrUtils::Trim(track_info);
    metadata->artist.clear();
    return true;
  }
  metadata->artist = StrUtils::Trim(track_info.substr(0, dash));
  metadata->title = StrUtils::Trim(track_info.substr(dash + 3));
  return true;
}

void M3UParser::ParsePlaylistData(const std::string &data, const std::string &dir, std::set<std::string> *ancestors, int depth,
                                  SongList *songs) const {
  Metadata current;
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::string trimmed = StrUtils::Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed[0] == '#') {
      ParseMetadata(trimmed, &current);
      continue;
    }
    if (IsNestedPlaylistReference(trimmed)) {
      current = Metadata();
      LoadNested(trimmed, dir, ancestors, depth, songs);
      continue;
    }
    Song song = LoadSong(dir, trimmed);
    if (!current.title.empty()) {
      song.set_title(current.title);
    }
    if (!current.artist.empty()) {
      song.set_artist(current.artist);
    }
    if (current.length_nanosec > 0) {
      song.set_length_nanosec(current.length_nanosec);
    }
    if (song.is_valid()) {
      songs->push_back(song);
    }
    current = Metadata();
  }
}

void M3UParser::LoadNested(const std::string &filename, const std::string &dir, std::set<std::string> *ancestors, int depth,
                           SongList *songs) const {
  if (depth >= kMaxNestingDepth) {
    LogWarning("Nested playlist depth cap reached, skipping: %s", filename.c_str());
    return;
  }
  std::string abs_path = filename;
  if (!abs_path.empty() && abs_path[0] != '/') {
    abs_path = FileUtils::Join(dir, abs_path);
  }
  if (ancestors && ancestors->count(abs_path)) {
    LogWarning("Nested playlist cycle detected, skipping: %s", abs_path.c_str());
    return;
  }
  if (!FileUtils::Exists(abs_path)) {
    LogWarning("Could not open nested playlist, skipping: %s", abs_path.c_str());
    return;
  }
  if (ancestors) {
    ancestors->insert(abs_path);
  }
  ParsePlaylistData(FileUtils::ReadFile(abs_path), FileUtils::DirName(abs_path), ancestors, depth + 1, songs);
  if (ancestors) {
    ancestors->erase(abs_path);
  }
}

SongList M3UParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  std::set<std::string> ancestors;
  if (!playlist_path.empty()) {
    ancestors.insert(playlist_path);
  }
  ParsePlaylistData(data, FileUtils::DirName(playlist_path), &ancestors, 0, &songs);
  return songs;
}

bool M3UParser::Save(const std::string &path, const SongList &songs) const {
  Settings settings;
  settings.BeginGroup("Playlist");
  const bool write_metadata = settings.BoolValue("write_metadata", true);
  std::string data = "#EXTM3U\n";
  const std::string dir = FileUtils::DirName(path);
  for (const Song &song : songs) {
    if (song.url().empty()) {
      continue;
    }
    if (write_metadata || song.is_stream()) {
      data += "#EXTINF:" + std::to_string(song.length_nanosec() / 1000000000LL) + "," + song.artist() + " - " + song.title() + "\n";
    }
    data += URLOrFilename(song.url(), dir) + "\n";
  }
  return FileUtils::WriteFile(path, data);
}
