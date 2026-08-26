#ifndef STRAWBERRY_GSTENGINEPIPELINE_H
#define STRAWBERRY_GSTENGINEPIPELINE_H

#include "core/song.h"

#include <gst/gst.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class GstEnginePipeline {
 public:
  explicit GstEnginePipeline(int id);
  ~GstEnginePipeline();

  GstEnginePipeline(const GstEnginePipeline &) = delete;
  GstEnginePipeline &operator=(const GstEnginePipeline &) = delete;

  int id() const { return id_; }
  bool valid() const { return playbin_ != nullptr; }

  bool Create(const std::string &url, const std::string &output, const std::string &device, uint64_t beginning_offset_nanosec,
              int64_t end_offset_nanosec, bool replaygain, int replaygain_mode, double replaygain_preamp, float stereo_balance);
  bool Play(bool pause, uint64_t offset_nanosec);
  void Stop();
  void Pause();
  void Unpause();
  void Seek(uint64_t offset_nanosec);
  void SetVolume(double fraction);
  void SetEqualizer(int preamp, const std::vector<int> &band_gains);
  void SetNextUri(const std::string &url);

  int64_t position_nanosec() const;
  int64_t length_nanosec() const;
  bool is_playing() const;

  std::function<void(int)> AboutToFinish;
  std::function<void(int)> EosReached;
  std::function<void(int)> StreamStarted;
  std::function<void(int, const std::string &)> ErrorOccurred;
  std::function<void(int, const std::vector<int16_t> &)> SpectrumReady;
  std::function<void(int, const Song &)> TagsReady;

 private:
  static gboolean BusCallback(GstBus *bus, GstMessage *message, gpointer data);
  static void AboutToFinishCb(GstElement *playbin, gpointer data);
  GstElement *MakeAudioSink(const std::string &output, const std::string &device) const;
  void HandleSpectrum(GstMessage *message);
  void HandleError(GstMessage *message);
  void HandleTags(GstMessage *message);

  int id_ = 0;
  GstElement *playbin_ = nullptr;
  GstElement *volume_ = nullptr;
  GstElement *equalizer_ = nullptr;
  guint bus_watch_id_ = 0;
  uint64_t beginning_offset_nanosec_ = 0;
  int64_t end_offset_nanosec_ = -1;
};

#endif  // STRAWBERRY_GSTENGINEPIPELINE_H
