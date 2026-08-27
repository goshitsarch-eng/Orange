#include "playlistparsers/asxiniparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <sstream>

bool AsxIniParser::TryMagic(const std::string &data) const { return StrUtils::ContainsInsensitive(data, "[reference]"); }

SongList AsxIniParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(playlist_path);
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::string trimmed = StrUtils::Trim(line);
    const auto eq = trimmed.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = StrUtils::ToLower(trimmed.substr(0, eq));
    if (!StrUtils::StartsWith(key, "ref")) {
      continue;
    }
    Song song = LoadSong(dir, trimmed.substr(eq + 1));
    if (song.is_valid()) {
      songs.push_back(song);
    }
  }
  return songs;
}

bool AsxIniParser::Save(const std::string &path, const SongList &songs) const {
  const std::string dir = FileUtils::DirName(path);
  std::string data = "[Reference]\n";
  int n = 1;
  for (const Song &song : songs) {
    data += "Ref" + std::to_string(n) + "=" + URLOrFilename(song.url(), dir) + "\n";
    ++n;
  }
  return FileUtils::WriteFile(path, data);
}
