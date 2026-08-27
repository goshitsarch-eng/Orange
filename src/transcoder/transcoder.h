#ifndef STRAWBERRY_TRANSCODER_H
#define STRAWBERRY_TRANSCODER_H

#include "core/song.h"
#include "core/signal.h"

#include <glib.h>
#include <gst/gst.h>

#include <map>
#include <memory>
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

  Transcoder();
  ~Transcoder();

  Transcoder(const Transcoder &) = delete;
  Transcoder &operator=(const Transcoder &) = delete;

  void AddJob(const Song &song, const std::string &destination, Format format);
  void set_quality(int quality) { quality_ = quality; }
  int quality() const { return quality_; }
  int max_threads() const { return max_threads_; }
  void set_max_threads(int count);
  void Start();
  void Cancel();
  bool cancelled() const { return cancelled_; }
  int finished_success() const { return finished_success_; }
  int finished_failed() const { return finished_failed_; }
  bool TranscodeFile(const Song &song, const std::string &destination, Format format);
  std::map<std::string, float> GetProgress() const;
  const std::vector<std::string> &log() const { return log_; }
  int job_count() const { return static_cast<int>(jobs_.size()); }
  int current_job_count() const { return static_cast<int>(current_jobs_.size()); }

  static std::string FormatName(Format format);
  static std::string Extension(Format format);
  static Preset PresetFor(Format format);
  static std::string PipelineFor(Format format, int quality = -1);

  Signal<int, int> Progress;
  Signal<> Finished;
  Signal<> AllJobsComplete;
  Signal<std::string, std::string, bool> JobComplete;
  Signal<std::string> LogLine;

 private:
  struct Job {
    Song song;
    std::string destination;
    Format format;
  };
  struct CurrentJob {
    Job job;
    GstElement *pipeline = nullptr;
  };
  struct FinishData {
    std::shared_ptr<bool> alive;
    Transcoder *self = nullptr;
    GstElement *pipeline = nullptr;
    bool success = false;
    int generation = 0;
    std::string error;
  };

  void PumpJobs();
  bool StartJob(const Job &job);
  void FinishJob(GstElement *pipeline, bool success, const std::string &error);
  GstElement *CreatePipeline(const Job &job, std::string *error);
  void StopCurrentJobs();
  static std::string JobInput(const Job &job);

  static gboolean BusWatch(GstBus *bus, GstMessage *msg, gpointer data);
  static gboolean FinishIdle(gpointer data);

  std::vector<Job> jobs_;
  std::vector<CurrentJob> current_jobs_;
  std::vector<std::string> log_;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  int quality_ = -1;
  int max_threads_ = 1;
  int total_ = 0;
  int finish_generation_ = 0;
  bool cancelled_ = false;
  int finished_success_ = 0;
  int finished_failed_ = 0;
};

#endif
