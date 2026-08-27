#ifndef STRAWBERRY_WPLPARSER_H
#define STRAWBERRY_WPLPARSER_H

#include "playlistparsers/parserbase.h"

class WplParser : public ParserBase {
 public:
  std::string name() const override { return "WPL"; }
  std::vector<std::string> file_extensions() const override { return {"wpl"}; }
  std::string mime_type() const override { return "application/vnd.ms-wpl"; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;
};

#endif
