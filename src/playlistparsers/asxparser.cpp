#include "playlistparsers/asxparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

bool ASXParser::TryMagic(const std::string &data) const { return StrUtils::ContainsInsensitive(data, "<asx"); }

Song ASXParser::ParseEntry(const std::string &entry_xml, const std::string &dir) {
  std::string href;
  const std::string lower = StrUtils::ToLower(entry_xml);
  size_t ref = lower.find("<ref");
  if (ref != std::string::npos) {
    const size_t end = entry_xml.find('>', ref);
    href = AttributeValue(entry_xml.substr(ref, end == std::string::npos ? std::string::npos : end - ref + 1), "href");
  }
  Song song = LoadSong(dir, href);
  const std::string title = TagText(entry_xml, "title", 0, nullptr);
  const std::string artist = TagText(entry_xml, "author", 0, nullptr);
  if (!title.empty()) {
    song.set_title(title);
  }
  if (!artist.empty()) {
    song.set_artist(artist);
  }
  return song;
}

SongList ASXParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(playlist_path);
  const std::string lower = StrUtils::ToLower(data);
  size_t pos = 0;
  while ((pos = lower.find("<entry", pos)) != std::string::npos) {
    const size_t gt = data.find('>', pos);
    if (gt == std::string::npos) {
      break;
    }
    const size_t end = lower.find("</entry>", gt);
    if (end == std::string::npos) {
      break;
    }
    Song song = ParseEntry(data.substr(gt + 1, end - (gt + 1)), dir);
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end + 8;
  }
  return songs;
}

bool ASXParser::Save(const std::string &path, const SongList &songs) const {
  std::string data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<asx version=\"3.0\">\n";
  for (const Song &song : songs) {
    data += "  <entry>\n    <title>" + XmlEscape(song.title()) + "</title>\n";
    data += "    <ref href=\"" + XmlEscape(song.url()) + "\"/>\n";
    if (!song.artist().empty()) {
      data += "    <author>" + XmlEscape(song.artist()) + "</author>\n";
    }
    data += "  </entry>\n";
  }
  data += "</asx>\n";
  return FileUtils::WriteFile(path, data);
}
