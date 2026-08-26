#ifndef STRAWBERRY_PLSPARSER_H
#define STRAWBERRY_PLSPARSER_H

#include "playlistparsers/parserbase.h"

class PLSParser : public ParserBase {
 public:
  std::string name() const override { return "PLS"; }
  std::vector<std::string> file_extensions() const override { return {"pls"}; }
  std::string mime_type() const override { return "audio/x-scpls"; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;
};

#endif
