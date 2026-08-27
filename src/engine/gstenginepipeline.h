#ifndef STRAWBERRY_GSTENGINEPIPELINE_H
#define STRAWBERRY_GSTENGINEPIPELINE_H

#include "core/song.h"

#include <gst/gst.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct GstPipelineExtras {
  double replaygain_fallback = 0.0;
  bool replaygain_compression = true;
  bool exclusive = false;
  bool volume_control = true;
  bool volume_exponential = false;
  bool channels_enabled = false;
  int channels = 0;
  bool bs2b = false;
  bool strict_ssl = false;
  std::string proxy_address;
  bool proxy_authentication = false;
  std::string proxy_user;
  std::string proxy_pass;
  int64_t buffer_duration_ms = 4000;
  double buffer_low_watermark = 0.33;
  double buffer_high_watermark = 0.99;
  int device_warmup_ms = 0;
};

class GstEnginePipeline {
 public:
  explicit GstEnginePipeline(int id);
  ~GstEnginePipeline();

  GstEnginePipeline(const GstEnginePipeline &) = delete;
  GstEnginePipeline &operator=(const GstEnginePipeline &) = delete;

  int id() const { return id_; }
  const std::string &url() const { return url_; }
  bool valid() const { return playbin_ != nullptr; }

  bool Create(const std::string &url, const std::string &output, const std::string &device, uint64_t beginning_offset_nanosec,
              int64_t end_offset_nanosec, bool replaygain, int replaygain_mode, double replaygain_preamp, float stereo_balance,
              bool playbin3 = false, const GstPipelineExtras &extras = {});
  bool Play(bool pause, uint64_t offset_nanosec);
  void Stop();
  void Pause();
  void Unpause();
  void Seek(uint64_t offset_nanosec);
  void SetVolume(double fraction);
  void SetEqualizer(int preamp, const std::vector<int> &band_gains);
  void SetStereoBalance(float value);
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
  void CancelWarmup();
  void HandleSpectrum(GstMessage *message);
  void HandleError(GstMessage *message);
  void HandleTags(GstMessage *message);

  int id_ = 0;
  std::string url_;
  GstElement *playbin_ = nullptr;
  GstElement *volume_ = nullptr;
  GstElement *equalizer_ = nullptr;
  GstElement *panorama_ = nullptr;
  guint bus_watch_id_ = 0;
  guint warmup_timeout_id_ = 0;
  uint64_t beginning_offset_nanosec_ = 0;
  int64_t end_offset_nanosec_ = -1;
  bool volume_control_ = true;
  bool strict_ssl_ = false;
  std::string proxy_address_;
  bool proxy_authentication_ = false;
  std::string proxy_user_;
  std::string proxy_pass_;
  int device_warmup_ms_ = 0;
};

#endif  // STRAWBERRY_GSTENGINEPIPELINE_H
