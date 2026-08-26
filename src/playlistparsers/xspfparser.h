#ifndef STRAWBERRY_XSPFPARSER_H
#define STRAWBERRY_XSPFPARSER_H

#include "playlistparsers/parserbase.h"

class XSPFParser : public ParserBase {
 public:
  std::string name() const override { return "XSPF"; }
  std::vector<std::string> file_extensions() const override { return {"xspf"}; }
  std::string mime_type() const override { return "application/xspf+xml"; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;

  static Song ParseTrack(const std::string &track_xml, const std::string &dir);
};

#endif
