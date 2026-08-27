#include "playlistparsers/xspfparser.h"

#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

bool XSPFParser::TryMagic(const std::string &data) const {
  return StrUtils::ContainsInsensitive(data, "<playlist") && StrUtils::ContainsInsensitive(data, "<tracklist");
}

Song XSPFParser::ParseTrack(const std::string &track_xml, const std::string &dir) {
  size_t next = 0;
  std::string location = TagText(track_xml, "location", 0, &next);
  if (location.empty()) {
    location = TagText(track_xml, "url", 0, nullptr);
  }
  Song song = LoadSong(dir, location);
  const std::string title = TagText(track_xml, "title", 0, nullptr);
  const std::string artist = TagText(track_xml, "creator", 0, nullptr);
  const std::string album = TagText(track_xml, "album", 0, nullptr);
  const std::string image = TagText(track_xml, "image", 0, nullptr);
  const std::string duration = TagText(track_xml, "duration", 0, nullptr);
  const std::string track_num = TagText(track_xml, "trackNum", 0, nullptr);
  if (!title.empty()) {
    song.set_title(title);
  }
  if (!artist.empty()) {
    song.set_artist(artist);
  }
  if (!album.empty()) {
    song.set_album(album);
  }
  if (!image.empty()) {
    song.set_art_manual(image);
  }
  if (!duration.empty()) {
    try {
      const long ms = std::stol(duration);
      if (ms > 0) {
        song.set_length_nanosec(ms * 1000000LL);
      }
    } catch (...) {
    }
  }
  if (!track_num.empty()) {
    try {
      const int track = std::stoi(track_num);
      if (track > 0) {
        song.set_track(track);
      }
    } catch (...) {
    }
  }
  return song;
}

SongList XSPFParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(playlist_path);
  const std::string lower = StrUtils::ToLower(data);
  size_t pos = 0;
  while ((pos = lower.find("<track", pos)) != std::string::npos) {
    const size_t gt = data.find('>', pos);
    if (gt == std::string::npos) {
      break;
    }
    const size_t end = lower.find("</track>", gt);
    if (end == std::string::npos) {
      break;
    }
    const std::string track_xml = data.substr(gt + 1, end - (gt + 1));
    Song song = ParseTrack(track_xml, dir);
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end + 8;
  }
  return songs;
}

bool XSPFParser::Save(const std::string &path, const SongList &songs) const {
  Settings settings;
  settings.BeginGroup("Playlist");
  const bool write_metadata = settings.BoolValue("write_metadata", true);
  const std::string dir = FileUtils::DirName(path);
  std::string data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n  <trackList>\n";
  for (const Song &song : songs) {
    data += "    <track>\n      <location>" + XmlEscape(URLOrFilename(song.url(), dir)) + "</location>\n";
    if (write_metadata || song.is_stream()) {
      data += "      <title>" + XmlEscape(song.title()) + "</title>\n";
      if (!song.artist().empty()) {
        data += "      <creator>" + XmlEscape(song.artist()) + "</creator>\n";
      }
      if (!song.album().empty()) {
        data += "      <album>" + XmlEscape(song.album()) + "</album>\n";
      }
      if (song.length_nanosec() > 0) {
        data += "      <duration>" + std::to_string(song.length_nanosec() / 1000000LL) + "</duration>\n";
      }
      if (song.track() > 0) {
        data += "      <trackNum>" + std::to_string(song.track()) + "</trackNum>\n";
      }
      if (!song.art_manual().empty()) {
        data += "      <image>" + XmlEscape(song.art_manual()) + "</image>\n";
      }
    }
    data += "    </track>\n";
  }
  data += "  </trackList>\n</playlist>\n";
  return FileUtils::WriteFile(path, data);
}
