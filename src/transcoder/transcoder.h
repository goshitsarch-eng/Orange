#ifndef STRAWBERRY_TRANSCODER_H
#define STRAWBERRY_TRANSCODER_H
#include "core/song.h"
#include "core/signal.h"
#include <string>
#include <vector>
class Transcoder {
 public:
  enum class Format { MP3, AAC, FLAC, OggVorbis, Opus, Speex, WavPack, ASF };
  void AddJob(const Song &song, const std::string &destination, Format format);
  void Start();
  void Cancel();
  static std::string FormatName(Format format);
  static std::string Extension(Format format);
  Signal<int, int> Progress;
  Signal<> Finished;
 private:
  struct Job { Song song; std::string destination; Format format; };
  std::vector<Job> jobs_;
};
#endif
