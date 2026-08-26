#include "playlistparsers/cueparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <cctype>
#include <cstdio>
#include <sstream>

bool CueParser::TryMagic(const std::string &data) const {
  return StrUtils::ContainsInsensitive(data, "\nfile ") || StrUtils::ContainsInsensitive(data, "\ntrack ") ||
         StrUtils::StartsWith(StrUtils::ToLower(StrUtils::Trim(data)), "file ") ||
         StrUtils::StartsWith(StrUtils::ToLower(StrUtils::Trim(data)), "performer ");
}

int64_t CueParser::IndexToNanosec(const std::string &index) {
  int minutes = 0;
  int seconds = 0;
  int frames = 0;
  if (std::sscanf(index.c_str(), "%d:%d:%d", &minutes, &seconds, &frames) < 2) {
    return 0;
  }
  const int64_t total_frames = (static_cast<int64_t>(minutes) * 60 + seconds) * 75 + frames;
  return total_frames * 1000000000LL / 75;
}

std::string CueParser::FindCueFilename(const std::string &audio_path) {
  const std::string dir = FileUtils::DirName(audio_path);
  const std::string base = FileUtils::BaseName(audio_path);
  const auto dot = base.rfind('.');
  const std::string stem = dot == std::string::npos ? base : base.substr(0, dot);
  for (const std::string &candidate : {FileUtils::Join(dir, stem + ".cue"), FileUtils::Join(dir, stem + ".CUE"), FileUtils::Join(dir, "album.cue")}) {
    if (FileUtils::Exists(candidate)) {
      return candidate;
    }
  }
  return {};
}

std::vector<std::string> CueParser::SplitCueLine(const std::string &line) {
  std::vector<std::string> parts;
  std::string current;
  bool in_quote = false;
  for (char c : line) {
    if (c == '"') {
      in_quote = !in_quote;
      continue;
    }
    if (!in_quote && std::isspace(static_cast<unsigned char>(c))) {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
      continue;
    }
    current.push_back(c);
  }
  if (!current.empty()) {
    parts.push_back(current);
  }
  return parts;
}

std::string CueParser::QuotedOrToken(const std::vector<std::string> &parts, size_t index) {
  if (index >= parts.size()) {
    return {};
  }
  return parts[index];
}

SongList CueParser::Load(const std::string &data, const std::string &playlist_path) const {
  SongList songs;
  const std::string dir = FileUtils::DirName(playlist_path);
  std::string file;
  std::string album;
  std::string album_artist;
  std::string album_composer;
  std::string album_genre;
  std::string album_date;
  std::string disc;
  Song current;
  bool in_track = false;
  auto finish_track = [&]() {
    if (current.is_valid()) {
      if (current.album().empty()) {
        current.set_album(album);
      }
      if (current.albumartist().empty()) {
        current.set_albumartist(album_artist);
      }
      if (current.composer().empty()) {
        current.set_composer(album_composer);
      }
      if (current.genre().empty()) {
        current.set_genre(album_genre);
      }
      if (current.year() <= 0 && !album_date.empty()) {
        try {
          current.set_year(std::stoi(album_date));
        } catch (...) {
        }
      }
      if (current.disc() <= 0 && !disc.empty()) {
        try {
          current.set_disc(std::stoi(disc));
        } catch (...) {
        }
      }
      current.set_cue_path(playlist_path);
      songs.push_back(current);
    }
    current = Song();
    in_track = false;
  };
  std::istringstream stream(data);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const std::vector<std::string> parts = SplitCueLine(StrUtils::Trim(line));
    if (parts.size() < 2) {
      continue;
    }
    const std::string name = StrUtils::ToUpper(parts[0]);
    const std::string value = parts[1];
    if (name == "FILE") {
      file = value[0] == '/' ? value : FileUtils::Join(dir, value);
    } else if (name == "TRACK") {
      finish_track();
      current = Song(Song::Source::LocalFile);
      current.set_url(FileUtils::UriFromPath(file));
      current.set_valid(!file.empty());
      try {
        current.set_track(std::stoi(value));
      } catch (...) {
      }
      in_track = true;
    } else if (name == "TITLE") {
      if (in_track) {
        current.set_title(value);
      } else {
        album = value;
      }
    } else if (name == "PERFORMER") {
      if (in_track) {
        current.set_artist(value);
      } else {
        album_artist = value;
      }
    } else if (name == "SONGWRITER" || name == "COMPOSER") {
      if (in_track) {
        current.set_composer(value);
      } else {
        album_composer = value;
      }
    } else if (name == "INDEX" && (value == "01" || value == "1") && parts.size() >= 3) {
      current.set_beginning_nanosec(IndexToNanosec(parts[2]));
    } else if (name == "REM" && parts.size() >= 3) {
      const std::string rem = StrUtils::ToUpper(value);
      if (rem == "GENRE") {
        if (in_track) {
          current.set_genre(parts[2]);
        } else {
          album_genre = parts[2];
        }
      } else if (rem == "DATE") {
        if (in_track) {
          try {
            current.set_year(std::stoi(parts[2]));
          } catch (...) {
          }
        } else {
          album_date = parts[2];
        }
      } else if (rem == "DISC" || rem == "DISCNUMBER") {
        disc = parts[2];
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

void CueParser::EnrichFromAudioFile(SongList *songs, const Song &file) {
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
    if (song.year() <= 0 && file.year() > 0) {
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
