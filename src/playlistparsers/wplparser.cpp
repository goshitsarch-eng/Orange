#include "playlistparsers/wplparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

bool WplParser::TryMagic(const std::string &data) const {
  return StrUtils::ContainsInsensitive(data, "<?wpl") || StrUtils::ContainsInsensitive(data, "<smil>");
}

SongList WplParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(playlist_path);
  const std::string lower = StrUtils::ToLower(data);
  size_t pos = 0;
  while ((pos = lower.find("<media", pos)) != std::string::npos) {
    const size_t end = data.find('>', pos);
    if (end == std::string::npos) {
      break;
    }
    const std::string src = AttributeValue(data.substr(pos, end - pos + 1), "src");
    Song song = LoadSong(dir, src);
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end + 1;
  }
  return songs;
}

bool WplParser::Save(const std::string &path, const SongList &songs) const {
  const std::string dir = FileUtils::DirName(path);
  std::string data = "<?wpl version=\"1.0\"?>\n<smil>\n  <head>\n";
  data += "    <meta name=\"Generator\" content=\"Strawberry\"/>\n";
  data += "    <meta name=\"ItemCount\" content=\"" + std::to_string(songs.size()) + "\"/>\n";
  data += "  </head>\n  <body>\n    <seq>\n";
  for (const Song &song : songs) {
    data += "      <media src=\"" + XmlEscape(URLOrFilename(song.url(), dir)) + "\"/>\n";
  }
  data += "    </seq>\n  </body>\n</smil>\n";
  return FileUtils::WriteFile(path, data);
}
