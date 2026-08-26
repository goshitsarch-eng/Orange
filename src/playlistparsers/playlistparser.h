#ifndef STRAWBERRY_PLAYLISTPARSER_H
#define STRAWBERRY_PLAYLISTPARSER_H

#include "core/song.h"
#include "playlistparsers/parserbase.h"

#include <memory>
#include <string>
#include <vector>

class PlaylistParser {
 public:
  PlaylistParser();

  SongList Load(const std::string &path) const;
  SongList LoadFromData(const std::string &data, const std::string &hint = {}) const;
  bool Save(const std::string &path, const SongList &songs) const;

  ParserBase *ParserForExtension(const std::string &suffix) const;
  ParserBase *ParserForMagic(const std::string &data) const;
  std::vector<ParserBase *> parsers() const;

  static bool IsPlaylist(const std::string &path);
  static std::vector<std::string> SupportedExtensions();
  static std::string FindCueForAudio(const std::string &audio_path);
  static int64_t CueIndexToNanosec(const std::string &index);
  static void EnrichFromAudioFile(SongList *songs, const Song &file);

 private:
  std::vector<std::unique_ptr<ParserBase>> parsers_;
};

#endif
