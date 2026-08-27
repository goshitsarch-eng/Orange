#ifndef STRAWBERRY_ORGANIZEFORMAT_H
#define STRAWBERRY_ORGANIZEFORMAT_H

#include "core/song.h"
#include "organize/organizefilename.h"

#include <string>
#include <utility>
#include <vector>

class OrganizeFormat {
 public:
  struct GetFilenameResult {
    std::string path;
    bool unique_filename = false;
  };

  explicit OrganizeFormat(std::string format = "%albumartist/%album{ - Disc %disc}/{%track - }%title");
  std::string GetFilenameForSong(const Song &song) const;
  GetFilenameResult GetFilenameForSongResult(const Song &song, const std::string &extension = {}) const;
  const std::string &format() const { return format_; }
  void set_format(const std::string &format) { format_ = format; }
  bool replace_spaces() const { return replace_spaces_; }
  void set_replace_spaces(bool v) { replace_spaces_ = v; }
  bool remove_problematic() const { return remove_problematic_; }
  void set_remove_problematic(bool v) { remove_problematic_ = v; }
  bool remove_non_fat() const { return remove_non_fat_; }
  void set_remove_non_fat(bool v) { remove_non_fat_ = v; }
  bool remove_non_ascii() const { return remove_non_ascii_; }
  void set_remove_non_ascii(bool v) { remove_non_ascii_ = v; }
  bool allow_ascii_ext() const { return allow_ascii_ext_; }
  void set_allow_ascii_ext(bool v) { allow_ascii_ext_ = v; }
  OrganizeFilename::Options FilenameOptions() const {
    OrganizeFilename::Options options;
    options.remove_problematic = remove_problematic_;
    options.remove_non_fat = remove_non_fat_;
    options.remove_non_ascii = remove_non_ascii_;
    options.allow_ascii_ext = allow_ascii_ext_;
    options.replace_spaces = replace_spaces_;
    return options;
  }
  bool IsValid() const;
  static bool TokenHasValue(const std::string &token, const Song &song);
  static std::string TokenValue(const std::string &token, const Song &song);
  static bool IsUniqueTag(const std::string &token);
  static std::string ArtistInitial(const std::string &albumartist);
  static const char *kKnownTags[];
  static std::vector<std::pair<const char *, const char *>> InsertTags() {
    return {{"Album", "album"},
            {"Album artist", "albumartist"},
            {"Artist", "artist"},
            {"Artist's initial", "artistinitial"},
            {"Bit depth", "bitdepth"},
            {"Bitrate", "bitrate"},
            {"Comment", "comment"},
            {"Composer", "composer"},
            {"Disc", "disc"},
            {"File extension", "extension"},
            {"Genre", "genre"},
            {"Grouping", "grouping"},
            {"Length", "length"},
            {"Original year", "originalyear"},
            {"Performer", "performer"},
            {"Sample rate", "samplerate"},
            {"Title", "title"},
            {"Track", "track"},
            {"Year", "year"}};
  }

 private:
  std::string ExpandTokens(const std::string &pattern, const Song &song) const;
  std::string ApplyFilenameFixes(std::string path, const Song &song) const;
  std::string format_;
  bool replace_spaces_ = false;
  bool remove_problematic_ = false;
  bool remove_non_fat_ = false;
  bool remove_non_ascii_ = false;
  bool allow_ascii_ext_ = false;
};

#endif
