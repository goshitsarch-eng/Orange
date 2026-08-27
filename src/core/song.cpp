#include "core/song.h"

#include "utilities/fileutils.h"

#include <glib.h>

#include <algorithm>
#include <cctype>

const std::vector<std::string> Song::kAcceptedExtensions = {
    "wav", "flac", "wv",  "ogg",  "oga", "opus", "spx", "ape", "mpc", "mp2",
    "mp3", "m4a",  "mp4", "aac",  "asf", "asx",  "wma", "aif", "aiff", "mka",
    "tta", "dsf",  "dsd", "dff",  "webm", "ac3", "dts",  "spc", "vgm", "tak",
    "mod", "s3m",  "xm",  "it"};

const std::vector<std::string> Song::kRejectedExtensions = {
    "tmp", "tar", "gz", "bz2", "xz", "tbz", "tgz", "z", "zip", "rar", "wvc", "zst", "lrc"};

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

void Song::InitFromFilePartial(const std::string &filename) {
  set_url(FileUtils::UriFromPath(filename));
  set_valid(true);
  set_source(Source::LocalFile);
  set_filetype(FiletypeByFilename(filename));
  const std::string base = FileUtils::BaseName(filename);
  set_basefilename(base);
  set_title(base);
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

bool Song::is_radio() const {
  return source_ == Source::Stream || source_ == Source::SomaFM || source_ == Source::RadioParadise || source_ == Source::RadioBrowser;
}

bool Song::is_stream_service() const {
  return source_ == Source::Subsonic || source_ == Source::Tidal || source_ == Source::Qobuz || source_ == Source::Spotify;
}

bool Song::is_stream() const { return is_radio() || is_stream_service() || filetype_ == FileType::Stream; }

bool Song::is_metadata_good() const { return !url_.empty() && !artist_.empty() && !title_.empty(); }

bool Song::IsEditable() const {
  return valid_ && !url_.empty() && ((url_is_local_file() && write_tags_supported() && !has_cue()) || is_stream());
}

void Song::MergeUserSetData(const Song &other, bool merge_playcount, bool merge_rating) {
  if (merge_playcount && other.playcount() > 0) {
    set_playcount(other.playcount());
  }
  if (merge_rating && other.rating() > 0.0f) {
    set_rating(other.rating());
  }
  set_skipcount(other.skipcount());
  set_lastplayed(other.lastplayed());
  set_art_manual(other.art_manual());
  set_art_unset(other.art_unset());
  set_compilation_on(other.compilation_on());
  set_compilation_off(other.compilation_off());
}

Song::FileType Song::FiletypeByExtension(const std::string &extension) {
  std::string ext = extension;
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
  if (ext == "wav" || ext == "wave") return FileType::WAV;
  if (ext == "flac") return FileType::FLAC;
  if (ext == "wv" || ext == "wavpack") return FileType::WavPack;
  if (ext == "ogg" || ext == "oga") return FileType::OggVorbis;
  if (ext == "opus") return FileType::OggOpus;
  if (ext == "spx" || ext == "speex") return FileType::OggSpeex;
  if (ext == "mp3" || ext == "mp2") return FileType::MPEG;
  if (ext == "m4a" || ext == "mp4" || ext == "aac") return FileType::MP4;
  if (ext == "wma" || ext == "asf" || ext == "asx") return FileType::ASF;
  if (ext == "aif" || ext == "aiff" || ext == "aifc") return FileType::AIFF;
  if (ext == "mpc" || ext == "mp+" || ext == "mpp") return FileType::MPC;
  if (ext == "tta") return FileType::TrueAudio;
  if (ext == "dsf") return FileType::DSF;
  if (ext == "dsd" || ext == "dff") return FileType::DSDIFF;
  if (ext == "ape") return FileType::APE;
  if (ext == "mod" || ext == "module" || ext == "nst" || ext == "wow") return FileType::MOD;
  if (ext == "s3m") return FileType::S3M;
  if (ext == "xm") return FileType::XM;
  if (ext == "it") return FileType::IT;
  if (ext == "spc") return FileType::SPC;
  if (ext == "vgm") return FileType::VGM;
  return FileType::Unknown;
}

Song::FileType Song::FiletypeByMimeType(const std::string &mimetype) {
  std::string mime = mimetype;
  const auto semicolon = mime.find(';');
  if (semicolon != std::string::npos) {
    mime = mime.substr(0, semicolon);
  }
  std::transform(mime.begin(), mime.end(), mime.begin(), [](unsigned char c) { return std::tolower(c); });
  if (mime == "audio/flac" || mime == "audio/x-flac") return FileType::FLAC;
  if (mime == "audio/mpeg" || mime == "audio/mp3" || mime == "audio/x-mpeg") return FileType::MPEG;
  if (mime == "audio/mp4" || mime == "audio/m4a" || mime == "audio/x-m4a" || mime == "audio/aac") return FileType::MP4;
  if (mime == "audio/ogg" || mime == "application/ogg" || mime == "audio/vorbis" || mime == "audio/x-vorbis") return FileType::OggVorbis;
  if (mime == "audio/opus" || mime == "audio/x-opus" || mime == "audio/ogg; codecs=opus") return FileType::OggOpus;
  if (mime == "audio/wav" || mime == "audio/x-wav" || mime == "audio/wave") return FileType::WAV;
  if (mime == "audio/x-wavpack") return FileType::WavPack;
  if (mime == "audio/x-ms-wma" || mime == "audio/wma" || mime == "audio/x-wma") return FileType::ASF;
  if (mime == "audio/aiff" || mime == "audio/x-aiff") return FileType::AIFF;
  if (mime == "audio/x-speex" || mime == "audio/speex") return FileType::OggSpeex;
  if (mime == "audio/x-musepack" || mime == "application/x-project") return FileType::MPC;
  if (mime == "audio/x-ape" || mime == "application/x-ape" || mime == "audio/x-ffmpeg-parsed-ape") return FileType::APE;
  if (mime == "audio/x-spc") return FileType::SPC;
  if (mime == "audio/x-vgm") return FileType::VGM;
  if (mime == "audio/x-alac") return FileType::ALAC;
  if (mime == "audio/x-dsf" || mime == "audio/dsf") return FileType::DSF;
  if (mime == "audio/x-dsd" || mime == "audio/x-dff" || mime == "audio/dsd") return FileType::DSDIFF;
  if (mime == "audio/x-mod" || mime == "audio/mod") return FileType::MOD;
  if (mime == "audio/x-s3m" || mime == "audio/s3m") return FileType::S3M;
  if (mime == "audio/x-xm") return FileType::XM;
  if (mime == "audio/x-it") return FileType::IT;
  if (mime == "audio/x-tta" || mime == "audio/tta") return FileType::TrueAudio;
  const auto slash = mime.rfind('/');
  if (slash != std::string::npos) {
    const FileType by_suffix = FiletypeByExtension(mime.substr(slash + 1));
    if (by_suffix != FileType::Unknown) {
      return by_suffix;
    }
  }
  return FileType::Unknown;
}

Song::FileType Song::FiletypeByDescription(const std::string &text) {
  if (g_ascii_strcasecmp(text.c_str(), "WAV") == 0) return FileType::WAV;
  if (g_ascii_strcasecmp(text.c_str(), "Free Lossless Audio Codec (FLAC)") == 0) return FileType::FLAC;
  if (g_ascii_strcasecmp(text.c_str(), "Wavpack") == 0) return FileType::WavPack;
  if (g_ascii_strcasecmp(text.c_str(), "Vorbis") == 0) return FileType::OggVorbis;
  if (g_ascii_strcasecmp(text.c_str(), "Opus") == 0) return FileType::OggOpus;
  if (g_ascii_strcasecmp(text.c_str(), "Speex") == 0) return FileType::OggSpeex;
  if (g_ascii_strcasecmp(text.c_str(), "MPEG-1 Layer 2 (MP2)") == 0) return FileType::MPEG;
  if (g_ascii_strcasecmp(text.c_str(), "MPEG-1 Layer 3 (MP3)") == 0) return FileType::MPEG;
  if (g_ascii_strcasecmp(text.c_str(), "MPEG-4 AAC") == 0) return FileType::MP4;
  if (g_ascii_strcasecmp(text.c_str(), "WMA") == 0) return FileType::ASF;
  if (g_ascii_strcasecmp(text.c_str(), "Audio Interchange File Format") == 0) return FileType::AIFF;
  if (g_ascii_strcasecmp(text.c_str(), "MPC") == 0) return FileType::MPC;
  if (g_ascii_strcasecmp(text.c_str(), "Musepack (MPC)") == 0) return FileType::MPC;
  if (g_ascii_strcasecmp(text.c_str(), "audio/x-dsf") == 0) return FileType::DSF;
  if (g_ascii_strcasecmp(text.c_str(), "audio/x-dsd") == 0) return FileType::DSDIFF;
  if (g_ascii_strcasecmp(text.c_str(), "audio/x-ffmpeg-parsed-ape") == 0) return FileType::APE;
  if (g_ascii_strcasecmp(text.c_str(), "Module Music Format (MOD)") == 0) return FileType::MOD;
  if (g_ascii_strcasecmp(text.c_str(), "Module Music Format (S3M)") == 0) return FileType::S3M;
  if (g_ascii_strcasecmp(text.c_str(), "SNES SPC700") == 0) return FileType::SPC;
  if (g_ascii_strcasecmp(text.c_str(), "VGM") == 0) return FileType::VGM;
  if (g_ascii_strcasecmp(text.c_str(), "Apple Lossless Audio Codec (ALAC)") == 0) return FileType::ALAC;
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

std::string Song::DomainForSource(Source source) {
  switch (source) {
    case Source::Tidal:
      return "tidal.com";
    case Source::Qobuz:
      return "qobuz.com";
    case Source::SomaFM:
      return "somafm.com";
    case Source::RadioParadise:
      return "radioparadise.com";
    case Source::RadioBrowser:
      return "radio-browser.info";
    case Source::Spotify:
      return "spotify.com";
    default:
      return {};
  }
}

std::string Song::ShareURL() const {
  switch (source_) {
    case Source::Stream:
    case Source::SomaFM:
    case Source::RadioBrowser:
      return url_;
    case Source::Tidal:
      return song_id_.empty() ? std::string() : "https://tidal.com/track/" + song_id_;
    case Source::Qobuz:
      return song_id_.empty() ? std::string() : "https://open.qobuz.com/track/" + song_id_;
    case Source::Spotify:
      return song_id_.empty() ? std::string() : "https://open.spotify.com/track/" + song_id_;
    default:
      return {};
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
      return "Module Music Format";
    case FileType::SPC:
      return "SNES SPC700";
    case FileType::VGM:
      return "VGM";
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

std::string Song::TitleRemoveMisc(const std::string &title) {
  static const char *kPatterns[] = {
      "\\s+-*\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*$",
      "\\s+-*\\s*\\(\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*\\)\\s*$",
      "\\s+-*\\s*\\[\\s*(([0-9]{4})*\\s*Remastered|([0-9]{4})*\\s*Remaster)\\s*(Version)*\\s*\\]\\s*$",
      "\\s+-*\\s*Explicit\\s*$",
      "\\s+-*\\s*\\(\\s*Explicit\\s*\\)\\s*$",
      "\\s+-*\\s*\\[\\s*Explicit\\s*\\]\\s*$",
  };
  std::string result = title;
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
