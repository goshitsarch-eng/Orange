#ifndef STRAWBERRY_ORGANIZE_H
#define STRAWBERRY_ORGANIZE_H

#include "core/song.h"

#include <string>
#include <vector>

class OrganizeFormat {
 public:
  explicit OrganizeFormat(std::string format = "%albumartist/%album{ - Disc %disc}/{%track - }%title");
  std::string GetFilenameForSong(const Song &song) const;
  const std::string &format() const { return format_; }
  void set_format(const std::string &format) { format_ = format; }
  static bool TokenHasValue(const std::string &token, const Song &song);

 private:
  std::string ExpandTokens(const std::string &pattern, const Song &song) const;
  std::string format_;
};

class Organize {
 public:
  struct Error {
    std::string song;
    std::string message;
  };

  std::vector<Error> Copy(const SongList &songs, const std::string &destination, const OrganizeFormat &format, bool move);
};

#endif
