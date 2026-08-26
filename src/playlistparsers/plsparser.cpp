#include "playlistparsers/plsparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <map>
#include <sstream>

bool PLSParser::TryMagic(const std::string &data) const { return StrUtils::ContainsInsensitive(data, "[playlist]"); }

SongList PLSParser::Load(const std::string &data, const std::string &playlist_path) const {
  std::map<int, Song> songs;
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
    const std::string value = trimmed.substr(eq + 1);
    int n = 0;
    size_t digit = key.find_first_of("0123456789");
    if (digit != std::string::npos) {
      try {
        n = std::stoi(key.substr(digit));
      } catch (...) {
        n = 0;
      }
    }
    if (StrUtils::StartsWith(key, "file")) {
      Song song = LoadSong(dir, value);
      if (!songs[n].title().empty()) {
        song.set_title(songs[n].title());
      }
      if (songs[n].length_nanosec() > 0) {
        song.set_length_nanosec(songs[n].length_nanosec());
      }
      songs[n] = song;
    } else if (StrUtils::StartsWith(key, "title")) {
      songs[n].set_title(value);
    } else if (StrUtils::StartsWith(key, "length")) {
      try {
        const long seconds = std::stol(value);
        if (seconds > 0) {
          songs[n].set_length_nanosec(seconds * 1000000000LL);
        }
      } catch (...) {
      }
    }
  }
  SongList result;
  for (auto &entry : songs) {
    if (entry.second.is_valid()) {
      result.push_back(entry.second);
    }
  }
  return result;
}

bool PLSParser::Save(const std::string &path, const SongList &songs) const {
  const std::string dir = FileUtils::DirName(path);
  std::string data = "[playlist]\nVersion=2\nNumberOfEntries=" + std::to_string(songs.size()) + "\n";
  int n = 1;
  for (const Song &song : songs) {
    data += "File" + std::to_string(n) + "=" + URLOrFilename(song.url(), dir) + "\n";
    data += "Title" + std::to_string(n) + "=" + song.title() + "\n";
    data += "Length" + std::to_string(n) + "=" + std::to_string(song.length_nanosec() / 1000000000LL) + "\n";
    ++n;
  }
  return FileUtils::WriteFile(path, data);
}
