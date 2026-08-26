#include "core/song.h"

#include "utilities/fileutils.h"

#include <glib.h>

#include <algorithm>
#include <cctype>

const std::vector<std::string> Song::kAcceptedExtensions = {
    "wav", "flac", "wv",  "ogg",  "oga", "opus", "spx", "ape", "mpc", "mp2",
    "mp3", "m4a",  "mp4", "aac",  "asf", "asx",  "wma", "aif", "aiff", "mka",
    "tta", "dsf",  "dsd", "webm", "ac3", "dts",  "spc", "vgm", "tak"};

const std::vector<std::string> Song::kRejectedExtensions = {
    "tmp", "tar", "gz", "bz2", "xz", "tbz", "tgz", "z", "zip", "rar"};

const std::vector<std::string> Song::kColumns = {
    "title", "titlesort", "album", "albumsort", "artist", "artistsort", "albumartist", "albumartistsort",
    "track", "disc", "year", "originalyear", "genre", "compilation", "composer", "composersort",
    "performer", "performersort", "grouping", "comment", "lyrics", "artist_id", "album_id", "song_id",
    "beginning", "length", "bitrate", "samplerate", "bitdepth", "source", "directory_id", "url",
    "filetype", "filesize", "mtime", "ctime", "unavailable", "fingerprint", "playcount", "skipcount",
    "lastplayed", "lastseen", "compilation_detected", "compilation_on", "compilation_off",
    "compilation_effective", "art_embedded", "art_automatic", "art_manual", "art_unset",
    "effective_albumartist", "effective_originalyear", "cue_path", "rating", "acoustid_id",
    "acoustid_fingerprint", "musicbrainz_album_artist_id", "musicbrainz_artist_id",
    "musicbrainz_original_artist_id", "musicbrainz_album_id", "musicbrainz_original_album_id",
    "musicbrainz_recording_id", "musicbrainz_track_id", "musicbrainz_disc_id",
    "musicbrainz_release_group_id", "musicbrainz_work_id", "ebur128_integrated_loudness_lufs",
    "ebur128_loudness_range_lu", "bpm", "mood", "initial_key"};

const char *Song::kColumnSpec =
    "title, titlesort, album, albumsort, artist, artistsort, albumartist, albumartistsort, "
    "track, disc, year, originalyear, genre, compilation, composer, composersort, "
    "performer, performersort, grouping, comment, lyrics, artist_id, album_id, song_id, "
    "beginning, length, bitrate, samplerate, bitdepth, source, directory_id, url, "
    "filetype, filesize, mtime, ctime, unavailable, fingerprint, playcount, skipcount, "
    "lastplayed, lastseen, compilation_detected, compilation_on, compilation_off, "
    "compilation_effective, art_embedded, art_automatic, art_manual, art_unset, "
    "effective_albumartist, effective_originalyear, cue_path, rating, acoustid_id, "
    "acoustid_fingerprint, musicbrainz_album_artist_id, musicbrainz_artist_id, "
    "musicbrainz_original_artist_id, musicbrainz_album_id, musicbrainz_original_album_id, "
    "musicbrainz_recording_id, musicbrainz_track_id, musicbrainz_disc_id, "
    "musicbrainz_release_group_id, musicbrainz_work_id, ebur128_integrated_loudness_lufs, "
    "ebur128_loudness_range_lu, bpm, mood, initial_key";

Song::Song(Source source) : source_(source) {}

bool Song::operator==(const Song &other) const {
  return url_ == other.url_ && beginning_nanosec_ == other.beginning_nanosec_;
}

void Song::set_url(const std::string &v) {
  url_ = v;
  if (basefilename_.empty()) {
    basefilename_ = FileUtils::BaseName(v);
  }
}

std::string Song::PrettyTitle() const {
  if (!title_.empty()) {
    return title_;
  }
  if (!basefilename_.empty()) {
    return basefilename_;
  }
  return url_;
}

std::string Song::PrettyTitleWithArtist() const {
  const std::string title = PrettyTitle();
  const std::string artist = EffectiveAlbumartist();
  if (artist.empty()) {
    return title;
  }
  return artist + " - " + title;
}

std::string Song::EffectiveAlbumartist() const {
  if (!albumartist_.empty()) {
    return albumartist_;
  }
  return artist_;
}

bool Song::is_stream() const {
  return source_ == Source::Stream || source_ == Source::Tidal || source_ == Source::Subsonic ||
         source_ == Source::Qobuz || source_ == Source::Spotify || source_ == Source::SomaFM ||
         source_ == Source::RadioParadise || source_ == Source::RadioBrowser || filetype_ == FileType::Stream;
}

Song::FileType Song::FiletypeByExtension(const std::string &extension) {
  std::string ext = extension;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
  if (ext == "wav") return FileType::WAV;
  if (ext == "flac") return FileType::FLAC;
  if (ext == "wv") return FileType::WavPack;
  if (ext == "ogg" || ext == "oga") return FileType::OggVorbis;
  if (ext == "opus") return FileType::OggOpus;
  if (ext == "spx") return FileType::OggSpeex;
  if (ext == "mp3" || ext == "mp2") return FileType::MPEG;
  if (ext == "m4a" || ext == "mp4" || ext == "aac") return FileType::MP4;
  if (ext == "wma" || ext == "asf" || ext == "asx") return FileType::ASF;
  if (ext == "aif" || ext == "aiff") return FileType::AIFF;
  if (ext == "mpc") return FileType::MPC;
  if (ext == "tta") return FileType::TrueAudio;
  if (ext == "dsf") return FileType::DSF;
  if (ext == "dsd") return FileType::DSDIFF;
  if (ext == "ape") return FileType::APE;
  if (ext == "spc") return FileType::SPC;
  if (ext == "vgm") return FileType::VGM;
  return FileType::Unknown;
}

Song::FileType Song::FiletypeByFilename(const std::string &filename) {
  return FiletypeByExtension(FileUtils::Extension(filename));
}

bool Song::IsAudioFile(const std::string &filename) {
  const std::string ext = FileUtils::Extension(filename);
  std::string lower = ext;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
  if (std::find(kRejectedExtensions.begin(), kRejectedExtensions.end(), lower) != kRejectedExtensions.end()) {
    return false;
  }
  return std::find(kAcceptedExtensions.begin(), kAcceptedExtensions.end(), lower) != kAcceptedExtensions.end();
}

std::string Song::SourceToString(Source source) {
  switch (source) {
    case Source::LocalFile:
      return "File";
    case Source::Collection:
      return "Collection";
    case Source::CDDA:
      return "CD";
    case Source::Device:
      return "Device";
    case Source::Stream:
      return "Stream";
    case Source::Tidal:
      return "Tidal";
    case Source::Subsonic:
      return "Subsonic";
    case Source::Qobuz:
      return "Qobuz";
    case Source::SomaFM:
      return "SomaFM";
    case Source::RadioParadise:
      return "Radio Paradise";
    case Source::Spotify:
      return "Spotify";
    case Source::RadioBrowser:
      return "Radio Browser";
    case Source::Unknown:
    default:
      return "Unknown";
  }
}

std::string Song::FiletypeToString(FileType type) {
  switch (type) {
    case FileType::WAV:
      return "WAV";
    case FileType::FLAC:
      return "FLAC";
    case FileType::WavPack:
      return "WavPack";
    case FileType::OggFlac:
      return "Ogg FLAC";
    case FileType::OggVorbis:
      return "Ogg Vorbis";
    case FileType::OggOpus:
      return "Opus";
    case FileType::OggSpeex:
      return "Speex";
    case FileType::MPEG:
      return "MP3";
    case FileType::MP4:
      return "MP4";
    case FileType::ASF:
      return "ASF";
    case FileType::AIFF:
      return "AIFF";
    case FileType::MPC:
      return "MPC";
    case FileType::TrueAudio:
      return "True Audio";
    case FileType::DSF:
      return "DSF";
    case FileType::DSDIFF:
      return "DSDIFF";
    case FileType::PCM:
      return "PCM";
    case FileType::APE:
      return "APE";
    case FileType::ALAC:
      return "ALAC";
    case FileType::CDDA:
      return "CDDA";
    case FileType::Stream:
      return "Stream";
    case FileType::MOD:
    case FileType::S3M:
    case FileType::XM:
    case FileType::IT:
    case FileType::SPC:
    case FileType::VGM:
      return "Module";
    case FileType::Unknown:
    default:
      return "Unknown";
  }
}

const char *Song::TextSearchColumnsSql() {
  return "title LIKE ? OR album LIKE ? OR artist LIKE ? OR albumartist LIKE ? OR composer LIKE ? OR performer LIKE ? OR grouping LIKE ? OR genre LIKE ? OR comment LIKE ?";
}

std::string Song::AlbumRemoveDiscMisc(const std::string &album) {
  static const char *kPatterns[] = {
      "\\s+-*\\s*(Disc|CD)\\s*([0-9]{1,2})$",
      "\\s+-*\\s*\\(\\s*(Disc|CD)\\s*([0-9]{1,2})\\)$",
      "\\s+-*\\s*\\[\\s*(Disc|CD)\\s*([0-9]{1,2})\\]$",
      "\\s+-*\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*$",
      "\\s+-*\\s*\\(\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*\\)\\s*$",
      "\\s+-*\\s*\\[\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*\\]\\s*$",
      "\\s+-*\\s*Explicit\\s*$",
      "\\s+-*\\s*\\(\\s*Explicit\\s*\\)\\s*$",
      "\\s+-*\\s*\\[\\s*Explicit\\s*\\]\\s*$",
  };
  std::string result = album;
  for (const char *pattern : kPatterns) {
    GError *error = nullptr;
    GRegex *regex = g_regex_new(pattern, static_cast<GRegexCompileFlags>(G_REGEX_CASELESS | G_REGEX_OPTIMIZE), static_cast<GRegexMatchFlags>(0), &error);
    if (!regex) {
      if (error) {
        g_error_free(error);
      }
      continue;
    }
    gchar *replaced = g_regex_replace(regex, result.c_str(), -1, 0, "", static_cast<GRegexMatchFlags>(0), nullptr);
    if (replaced) {
      result = replaced;
      g_free(replaced);
    }
    g_regex_unref(regex);
  }
  while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) {
    result.pop_back();
  }
  return result;
}
