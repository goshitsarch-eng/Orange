#include "playlistparsers/playlistparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

bool PlaylistParser::IsPlaylist(const std::string &path) {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  const auto supported = SupportedExtensions();
  return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::vector<std::string> PlaylistParser::SupportedExtensions() {
  return {"m3u", "m3u8", "pls", "xspf", "asx", "asxini", "wpl", "cue"};
}

Song PlaylistParser::SongFromPath(const std::string &playlist_dir, const std::string &entry) const {
  Song song;
  std::string path = StrUtils::Trim(entry);
  if (path.empty()) {
    return song;
  }
  if (path.find("://") != std::string::npos) {
    song.set_source(path.rfind("file://", 0) == 0 ? Song::Source::LocalFile : Song::Source::Stream);
    song.set_url(path);
    song.set_title(FileUtils::BaseName(FileUtils::PathFromUri(path)));
    song.set_valid(true);
    return song;
  }
  if (!path.empty() && path[0] != '/') {
    path = FileUtils::Join(playlist_dir, path);
  }
  song.set_source(Song::Source::LocalFile);
  song.set_url(FileUtils::UriFromPath(path));
  song.set_title(FileUtils::BaseName(path));
  song.set_valid(true);
  return song;
}

SongList PlaylistParser::LoadM3U(const std::string &path, const std::string &data) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(path);
  std::string pending_title;
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::string trimmed = StrUtils::Trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
      if (StrUtils::StartsWith(trimmed, "#EXTINF:")) {
        const auto comma = trimmed.find(',');
        if (comma != std::string::npos) {
          pending_title = trimmed.substr(comma + 1);
        }
      }
      continue;
    }
    if (IsPlaylist(trimmed) && trimmed.find("://") == std::string::npos) {
      const std::string nested = trimmed[0] == '/' ? trimmed : FileUtils::Join(dir, trimmed);
      SongList nested_songs = Load(nested);
      songs.insert(songs.end(), nested_songs.begin(), nested_songs.end());
      pending_title.clear();
      continue;
    }
    Song song = SongFromPath(dir, trimmed);
    if (!pending_title.empty()) {
      song.set_title(pending_title);
      pending_title.clear();
    }
    if (song.is_valid()) {
      songs.push_back(song);
    }
  }
  return songs;
}

SongList PlaylistParser::LoadPLS(const std::string &data) const {
  SongList songs;
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    const std::string trimmed = StrUtils::Trim(line);
    const std::string lower = StrUtils::ToLower(trimmed);
    if (StrUtils::StartsWith(lower, "file")) {
      const auto eq = trimmed.find('=');
      if (eq != std::string::npos) {
        Song song = SongFromPath({}, trimmed.substr(eq + 1));
        if (song.is_valid()) {
          songs.push_back(song);
        }
      }
    }
  }
  return songs;
}

SongList PlaylistParser::LoadXSPF(const std::string &data) const {
  SongList songs;
  size_t pos = 0;
  while ((pos = data.find("<location>", pos)) != std::string::npos) {
    const size_t start = pos + 10;
    const size_t end = data.find("</location>", start);
    if (end == std::string::npos) {
      break;
    }
    Song song = SongFromPath({}, data.substr(start, end - start));
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end;
  }
  return songs;
}

SongList PlaylistParser::LoadASX(const std::string &data) const {
  SongList songs;
  size_t pos = 0;
  while ((pos = data.find("href=\"", pos)) != std::string::npos) {
    const size_t start = pos + 6;
    const size_t end = data.find('"', start);
    if (end == std::string::npos) {
      break;
    }
    Song song = SongFromPath({}, data.substr(start, end - start));
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end;
  }
  return songs;
}

SongList PlaylistParser::LoadWPL(const std::string &data) const {
  SongList songs;
  size_t pos = 0;
  while ((pos = data.find("src=\"", pos)) != std::string::npos) {
    const size_t start = pos + 5;
    const size_t end = data.find('"', start);
    if (end == std::string::npos) {
      break;
    }
    Song song = SongFromPath({}, data.substr(start, end - start));
    if (song.is_valid()) {
      songs.push_back(song);
    }
    pos = end;
  }
  return songs;
}

int64_t PlaylistParser::CueIndexToNanosec(const std::string &index) {
  int minutes = 0;
  int seconds = 0;
  int frames = 0;
  if (std::sscanf(index.c_str(), "%d:%d:%d", &minutes, &seconds, &frames) < 2) {
    return 0;
  }
  const int64_t total_frames = (static_cast<int64_t>(minutes) * 60 + seconds) * 75 + frames;
  return total_frames * 1000000000LL / 75;
}

std::string PlaylistParser::FindCueForAudio(const std::string &audio_path) {
  const std::string dir = FileUtils::DirName(audio_path);
  const std::string base = FileUtils::BaseName(audio_path);
  const auto dot = base.rfind('.');
  const std::string stem = dot == std::string::npos ? base : base.substr(0, dot);
  const std::vector<std::string> candidates = {FileUtils::Join(dir, stem + ".cue"), FileUtils::Join(dir, stem + ".CUE"),
                                               FileUtils::Join(dir, "album.cue")};
  for (const std::string &candidate : candidates) {
    if (FileUtils::Exists(candidate)) {
      return candidate;
    }
  }
  return {};
}

namespace {

std::string CueQuoted(const std::string &line) {
  const auto first = line.find('"');
  const auto last = line.rfind('"');
  if (first != std::string::npos && last > first) {
    return line.substr(first + 1, last - first - 1);
  }
  return {};
}

}  // namespace

SongList PlaylistParser::LoadCUE(const std::string &path, const std::string &data) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(path);
  std::string file;
  std::string album;
  std::string album_artist;
  Song current;
  bool in_track = false;
  std::istringstream stream(data);
  std::string line;
  auto finish_track = [&]() {
    if (current.is_valid()) {
      if (current.album().empty()) {
        current.set_album(album);
      }
      if (current.albumartist().empty()) {
        current.set_albumartist(album_artist);
      }
      current.set_cue_path(path);
      songs.push_back(current);
    }
    current = Song();
    in_track = false;
  };
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::string trimmed = StrUtils::Trim(line);
    if (StrUtils::StartsWith(trimmed, "FILE ")) {
      const std::string name = CueQuoted(trimmed);
      if (!name.empty()) {
        file = name[0] == '/' ? name : FileUtils::Join(dir, name);
      }
    } else if (StrUtils::StartsWith(trimmed, "TRACK ")) {
      finish_track();
      current = Song(Song::Source::LocalFile);
      current.set_url(FileUtils::UriFromPath(file));
      current.set_valid(!file.empty());
      int track = 0;
      std::sscanf(trimmed.c_str(), "TRACK %d", &track);
      current.set_track(track);
      in_track = true;
    } else if (StrUtils::StartsWith(trimmed, "TITLE ")) {
      const std::string title = CueQuoted(trimmed);
      if (in_track) {
        current.set_title(title);
      } else {
        album = title;
      }
    } else if (StrUtils::StartsWith(trimmed, "PERFORMER ")) {
      const std::string performer = CueQuoted(trimmed);
      if (in_track) {
        current.set_artist(performer);
      } else {
        album_artist = performer;
      }
    } else if (StrUtils::StartsWith(trimmed, "INDEX 01 ") || StrUtils::StartsWith(trimmed, "INDEX 1 ")) {
      const auto space = trimmed.rfind(' ');
      if (space != std::string::npos) {
        current.set_beginning_nanosec(CueIndexToNanosec(trimmed.substr(space + 1)));
      }
    }
  }
  finish_track();
  for (size_t i = 0; i + 1 < songs.size(); ++i) {
    const int64_t next = songs[i + 1].beginning_nanosec();
    const int64_t start = songs[i].beginning_nanosec();
    if (next > start) {
      songs[i].set_length_nanosec(next - start);
    }
  }
  return songs;
}

void PlaylistParser::EnrichFromAudioFile(SongList *songs, const Song &file) {
  if (!songs) {
    return;
  }
  for (Song &song : *songs) {
    song.set_bitrate(file.bitrate());
    song.set_samplerate(file.samplerate());
    song.set_bitdepth(file.bitdepth());
    song.set_filesize(file.filesize());
    song.set_filetype(file.filetype());
    if (song.basefilename().empty()) {
      song.set_basefilename(file.basefilename());
    }
    if (file.art_embedded()) {
      song.set_art_embedded(true);
    }
    if (song.year() == 0 && file.year() > 0) {
      song.set_year(file.year());
    }
    if (song.genre().empty() && !file.genre().empty()) {
      song.set_genre(file.genre());
    }
    if (song.albumartist().empty() && !file.albumartist().empty()) {
      song.set_albumartist(file.albumartist());
    }
    if (song.album().empty() && !file.album().empty()) {
      song.set_album(file.album());
    }
  }
  if (!songs->empty() && songs->back().length_nanosec() <= 0 && file.length_nanosec() > songs->back().beginning_nanosec()) {
    songs->back().set_length_nanosec(file.length_nanosec() - songs->back().beginning_nanosec());
  }
}

SongList PlaylistParser::Load(const std::string &path) const {
  const std::string data = FileUtils::ReadFile(path);
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  if (ext == "m3u" || ext == "m3u8") return LoadM3U(path, data);
  if (ext == "pls") return LoadPLS(data);
  if (ext == "xspf") return LoadXSPF(data);
  if (ext == "asx" || ext == "asxini") return LoadASX(data);
  if (ext == "wpl") return LoadWPL(data);
  if (ext == "cue") return LoadCUE(path, data);
  return {};
}

bool PlaylistParser::SaveM3U(const std::string &path, const SongList &songs) const {
  std::string data = "#EXTM3U\n";
  for (const Song &song : songs) {
    data += "#EXTINF:" + std::to_string(song.length_nanosec() / 1000000000LL) + "," + song.PrettyTitleWithArtist() + "\n";
    data += FileUtils::PathFromUri(song.url()) + "\n";
  }
  return FileUtils::WriteFile(path, data);
}

bool PlaylistParser::SaveXSPF(const std::string &path, const SongList &songs) const {
  std::string data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n  <trackList>\n";
  for (const Song &song : songs) {
    data += "    <track><location>" + song.url() + "</location><title>" + song.title() + "</title></track>\n";
  }
  data += "  </trackList>\n</playlist>\n";
  return FileUtils::WriteFile(path, data);
}

bool PlaylistParser::Save(const std::string &path, const SongList &songs) const {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(path));
  if (ext == "xspf") {
    return SaveXSPF(path, songs);
  }
  return SaveM3U(path, songs);
}
