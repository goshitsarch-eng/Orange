#ifndef STRAWBERRY_ORGANIZEFORMAT_H
#define STRAWBERRY_ORGANIZEFORMAT_H

#include "core/song.h"

#include <string>
#include <vector>

class OrganizeFormat {
 public:
  explicit OrganizeFormat(std::string format = "%albumartist/%album{ - Disc %disc}/{%track - }%title");
  std::string GetFilenameForSong(const Song &song) const;
  const std::string &format() const { return format_; }
  void set_format(const std::string &format) { format_ = format; }
  bool IsValid() const;
  static bool TokenHasValue(const std::string &token, const Song &song);
  static const char *kKnownTags[];

 private:
  std::string ExpandTokens(const std::string &pattern, const Song &song) const;
  std::string format_;
};

#endif
