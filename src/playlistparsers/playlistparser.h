#ifndef STRAWBERRY_PLAYLISTPARSER_H
#define STRAWBERRY_PLAYLISTPARSER_H

#include "core/song.h"

#include <string>
#include <vector>

class PlaylistParser {
 public:
  SongList Load(const std::string &path) const;
  bool Save(const std::string &path, const SongList &songs) const;

  static bool IsPlaylist(const std::string &path);
  static std::vector<std::string> SupportedExtensions();
  static std::string FindCueForAudio(const std::string &audio_path);
  static int64_t CueIndexToNanosec(const std::string &index);
  static void EnrichFromAudioFile(SongList *songs, const Song &file);

 private:
  SongList LoadM3U(const std::string &path, const std::string &data) const;
  SongList LoadPLS(const std::string &data) const;
  SongList LoadXSPF(const std::string &data) const;
  SongList LoadASX(const std::string &data) const;
  SongList LoadWPL(const std::string &data) const;
  SongList LoadCUE(const std::string &path, const std::string &data) const;
  bool SaveM3U(const std::string &path, const SongList &songs) const;
  bool SaveXSPF(const std::string &path, const SongList &songs) const;
  Song SongFromPath(const std::string &playlist_dir, const std::string &entry) const;
};

#endif  // STRAWBERRY_PLAYLISTPARSER_H
