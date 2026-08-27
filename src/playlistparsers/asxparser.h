#ifndef STRAWBERRY_ASXPARSER_H
#define STRAWBERRY_ASXPARSER_H

#include "playlistparsers/parserbase.h"

class ASXParser : public ParserBase {
 public:
  std::string name() const override { return "ASX"; }
  std::vector<std::string> file_extensions() const override { return {"asx"}; }
  std::string mime_type() const override { return "video/x-ms-asf"; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;

  static Song ParseEntry(const std::string &entry_xml, const std::string &dir);
};

#endif
