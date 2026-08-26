#ifndef STRAWBERRY_CUEPARSER_H
#define STRAWBERRY_CUEPARSER_H

#include "playlistparsers/parserbase.h"

class CueParser : public ParserBase {
 public:
  std::string name() const override { return "CUE"; }
  std::vector<std::string> file_extensions() const override { return {"cue"}; }
  std::string mime_type() const override { return "application/x-cue"; }
  bool TryMagic(const std::string &data) const override;
  SongList Load(const std::string &data, const std::string &playlist_path = {}) const override;

  static int64_t IndexToNanosec(const std::string &index);
  static std::string FindCueFilename(const std::string &audio_path);
  static std::vector<std::string> SplitCueLine(const std::string &line);
  static void EnrichFromAudioFile(SongList *songs, const Song &file);

 private:
  static std::string QuotedOrToken(const std::vector<std::string> &parts, size_t index);
};

#endif
