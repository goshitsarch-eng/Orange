#ifndef STRAWBERRY_TRANSCODER_H
#define STRAWBERRY_TRANSCODER_H

#include "core/song.h"
#include "core/signal.h"

#include <string>
#include <vector>

class Transcoder {
 public:
  enum class Format { MP3, AAC, FLAC, OggVorbis, Opus, Speex, WavPack, ASF };

  struct Preset {
    Format format = Format::FLAC;
    std::string name;
    std::string extension;
    std::string codec_mimetype;
    std::string muxer_mimetype;
  };

  void AddJob(const Song &song, const std::string &destination, Format format);
  void set_quality(int quality) { quality_ = quality; }
  int quality() const { return quality_; }
  void Start();
  void Cancel();
  bool cancelled() const { return cancelled_; }
  int finished_success() const { return finished_success_; }
  int finished_failed() const { return finished_failed_; }
  bool TranscodeFile(const Song &song, const std::string &destination, Format format);
  const std::vector<std::string> &log() const { return log_; }
  int job_count() const { return static_cast<int>(jobs_.size()); }

  static std::string FormatName(Format format);
  static std::string Extension(Format format);
  static Preset PresetFor(Format format);
  static std::string PipelineFor(Format format, int quality);

  Signal<int, int> Progress;
  Signal<> Finished;
  Signal<std::string> LogLine;

 private:
  struct Job {
    Song song;
    std::string destination;
    Format format;
  };
  std::vector<Job> jobs_;
  std::vector<std::string> log_;
  int quality_ = 5;
  bool cancelled_ = false;
  int finished_success_ = 0;
  int finished_failed_ = 0;
};

#endif
