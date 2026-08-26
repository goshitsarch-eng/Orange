#ifndef STRAWBERRY_M3UPARSER_H
#define STRAWBERRY_M3UPARSER_H

#include "playlistparsers/parserbase.h"

#include <set>
#include <string>

class M3UParser : public ParserBase {
 public:
  struct Metadata {
    std::string artist;
    std::string title;
    int64_t length_nanosec = -1;
  };

  static const int kMaxNestingDepth;

  std::string name() const override { return "M3U"; }
  std::vector<std::string> file_extensions() const override { return {"m3u", "m3u8"}; }
  std::string mime_type() const override { return "text/uri-list"; }
  bool save_supported() const override { return true; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;
  bool Save(const std::string &path, const SongList &songs) const override;

  static bool ParseMetadata(const std::string &line, Metadata *metadata);
  static bool IsNestedPlaylistReference(const std::string &line);

 private:
  void ParsePlaylistData(const std::string &data, const std::string &dir, std::set<std::string> *ancestors, int depth,
                         SongList *songs) const;
  void LoadNested(const std::string &filename, const std::string &dir, std::set<std::string> *ancestors, int depth,
                  SongList *songs) const;
};

#endif
