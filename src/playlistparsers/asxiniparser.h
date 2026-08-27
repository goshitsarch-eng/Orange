#ifndef STRAWBERRY_ASXINIPARSER_H
#define STRAWBERRY_ASXINIPARSER_H

#include "playlistparsers/parserbase.h"

class AsxIniParser : public ParserBase {
 public:
  std::string name() const override { return "ASX/INI"; }
  std::vector<std::string> file_extensions() const override { return {"asxini"}; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;
};

#endif
