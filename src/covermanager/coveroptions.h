#ifndef STRAWBERRY_COVEROPTIONS_H
#define STRAWBERRY_COVEROPTIONS_H

#include "constants/coverssettings.h"
#include "core/settings.h"
#include "core/song.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <cstdlib>
#include <string>

struct CoverOptions {
  enum class CoverType {
    Cache = 1,
    Album = 2,
    Embedded = 3
  };

  enum class CoverFilename {
    Hash = 1,
    Pattern = 2
  };

  int desired_height = 300;
  bool pad = false;
  bool scale = true;
  std::string default_cover;
  CoverType cover_type = CoverType::Cache;
  CoverFilename cover_filename = CoverFilename::Hash;
  std::string cover_pattern = CoversSettings::kDefaultSavePattern;
  bool cover_overwrite = CoversSettings::kDefaultSaveOverwrite;
  bool cover_lowercase = CoversSettings::kDefaultSaveLowercase;
  bool cover_replace_spaces = CoversSettings::kDefaultSaveReplaceSpaces;

  static CoverType TypeFromValue(const std::string &value) {
    if (value == "cache" || value == "0") {
      return CoverType::Cache;
    }
    if (value == "album") {
      return CoverType::Album;
    }
    if (value == "embedded") {
      return CoverType::Embedded;
    }
    char *end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end && end != value.c_str()) {
      if (parsed == static_cast<long>(CoverType::Album)) {
        return CoverType::Album;
      }
      if (parsed == static_cast<long>(CoverType::Embedded)) {
        return CoverType::Embedded;
      }
    }
    return CoverType::Cache;
  }

  static CoverFilename FilenameModeFromValue(const std::string &value) {
    if (value == "pattern") {
      return CoverFilename::Pattern;
    }
    if (value == "hash" || value == "0") {
      return CoverFilename::Hash;
    }
    char *end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end && end != value.c_str() && parsed == static_cast<long>(CoverFilename::Pattern)) {
      return CoverFilename::Pattern;
    }
    return CoverFilename::Hash;
  }

  static CoverOptions LoadFromSettings() {
    CoverOptions options;
    Settings settings;
    settings.BeginGroup(CoversSettings::kSettingsGroup);
    options.cover_type = TypeFromValue(settings.Value(CoversSettings::kSaveType, "2"));
    options.cover_filename = FilenameModeFromValue(settings.Value(CoversSettings::kSaveFilename, "2"));
    options.cover_pattern = settings.Value(CoversSettings::kSavePattern, CoversSettings::kDefaultSavePattern);
    options.cover_overwrite = settings.BoolValue(CoversSettings::kSaveOverwrite, CoversSettings::kDefaultSaveOverwrite);
    options.cover_lowercase = settings.BoolValue(CoversSettings::kSaveLowercase, CoversSettings::kDefaultSaveLowercase);
    options.cover_replace_spaces = settings.BoolValue(CoversSettings::kSaveReplaceSpaces, CoversSettings::kDefaultSaveReplaceSpaces);
    return options;
  }

  CoverType EffectiveType(const Song &song) const {
    if (cover_type == CoverType::Embedded && !song.save_embedded_cover_supported()) {
      return CoverType::Cache;
    }
    return cover_type;
  }

  static std::string CacheDirectory() {
    return FileUtils::Join(g_get_user_cache_dir(), "strawberry/covers");
  }

  static std::string HashHex(const std::string &artist, const std::string &album) {
    const std::string seed = StrUtils::ToLower(artist) + StrUtils::ToLower(album);
    gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_SHA1, seed.c_str(), static_cast<gssize>(seed.size()));
    std::string hex = digest ? digest : "";
    g_free(digest);
    return hex;
  }

  static std::string FilenameFromPattern(const std::string &pattern, const Song &song) {
    std::string filename = pattern;
    const std::string artist = song.EffectiveAlbumartist();
    const std::string album = Song::AlbumRemoveDiscMisc(song.album());
    filename = StrUtils::Replace(filename, "%albumartist", artist);
    filename = StrUtils::Replace(filename, "%artist", artist);
    filename = StrUtils::Replace(filename, "%album", album);
    return filename;
  }

  std::string DirectoryForSong(const Song &song) const {
    if (EffectiveType(song) == CoverType::Album) {
      const std::string dir = FileUtils::DirName(FileUtils::PathFromUri(song.url()));
      return dir.empty() ? "." : dir;
    }
    return CacheDirectory();
  }

  std::string FilenameForSong(const Song &song, const std::string &extension = "jpg") const {
    std::string filename;
    if (EffectiveType(song) == CoverType::Album && cover_filename == CoverFilename::Pattern && !cover_pattern.empty()) {
      filename = FilenameFromPattern(cover_pattern, song);
      if (cover_lowercase) {
        filename = StrUtils::ToLower(filename);
      }
      if (cover_replace_spaces) {
        filename = StrUtils::Replace(filename, " ", "-");
      }
    }
    if (filename.empty()) {
      filename = HashHex(song.EffectiveAlbumartist(), song.album());
    }
    if (filename.empty()) {
      filename = "cover";
    }
    if (!extension.empty()) {
      filename += "." + extension;
    }
    return filename;
  }

  std::string FilePath(const Song &song, const std::string &extension = "jpg") const {
    return FileUtils::Join(DirectoryForSong(song), FilenameForSong(song, extension));
  }

  static std::string UniquePath(std::string path, bool overwrite) {
    if (overwrite || !FileUtils::Exists(path)) {
      return path;
    }
    const std::string dir = FileUtils::DirName(path);
    std::string name = FileUtils::BaseName(path);
    while (FileUtils::Exists(path)) {
      name = "0" + name;
      path = FileUtils::Join(dir, name);
    }
    return path;
  }
};

#endif
