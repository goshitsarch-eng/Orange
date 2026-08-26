#ifndef STRAWBERRY_GSTENGINE_H
#define STRAWBERRY_GSTENGINE_H

#include "core/signal.h"
#include "core/song.h"

#include <gst/gst.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class GstEngine {
 public:
  enum class State { Empty, Idle, Playing, Paused, Error };

  enum TrackChangeType { First = 0x01, Manual = 0x02, Auto = 0x04, Intro = 0x08, SameAlbum = 0x10 };

  struct OutputDetails {
    std::string name;
    std::string description;
    std::string iconname;
  };

  GstEngine();
  ~GstEngine();

  bool Init();
  State state() const { return state_; }

  bool Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool force_stop_at_end,
            uint64_t beginning_offset_nanosec, int64_t end_offset_nanosec, std::optional<double> ebur128_lufs);
  bool Play(bool pause, uint64_t offset_nanosec);
  void Stop(bool stop_after = false);
  void Pause();
  void Unpause();
  void Seek(uint64_t offset_nanosec);
  void SetVolumeSW(unsigned percent);

  int64_t position_nanosec() const;
  int64_t length_nanosec() const;

  std::vector<OutputDetails> GetOutputsList() const;
  bool ValidOutput(const std::string &output) const;
  std::string DefaultOutput() const;
  void SetOutput(const std::string &output, const std::string &device);

  void SetEqualizerEnabled(bool enabled);
  void SetEqualizerParameters(int preamp, const std::vector<int> &band_gains);
  void SetReplayGainEnabled(bool enabled);
  void SetReplayGainMode(int mode);
  void SetReplayGainPreamp(double preamp);
  void SetStereoBalance(float value);
  void SetFadingEnabled(bool enabled);
  void SetFadeDurationMs(int milliseconds);
  const std::vector<int16_t> &last_scope() const { return last_scope_; }

  Signal<State> StateChanged;
  Signal<int64_t, int64_t> PositionChanged;
  Signal<> TrackEnded;
  Signal<> TrackAboutToEnd;
  Signal<std::string> Error;
  Signal<Song> MetadataReceived;
  Signal<std::vector<int16_t>> ScopeUpdated;

 private:
  static gboolean BusCallback(GstBus *bus, GstMessage *message, gpointer data);
  static void AboutToFinish(GstElement *playbin, gpointer data);
  static gboolean FadeTick(gpointer data);
  void HandleError(GstMessage *message);
  void HandleSpectrum(GstMessage *message);
  void SetState(State state);
  GstElement *MakeAudioSink() const;
  void ApplyVolume(double fraction);
  void StartFade(int direction);
  void CancelFade();

  GstElement *pipeline_ = nullptr;
  GstElement *playbin_ = nullptr;
  GstElement *volume_ = nullptr;
  GstElement *equalizer_ = nullptr;
  GstElement *rgvolume_ = nullptr;
  GstElement *rglimiter_ = nullptr;
  GstElement *panorama_ = nullptr;
  GstElement *spectrum_ = nullptr;
  int replaygain_mode_ = 0;
  double replaygain_preamp_ = 0.0;
  float stereo_balance_ = 0.0f;
  guint bus_watch_id_ = 0;
  State state_ = State::Empty;
  std::string output_ = "autoaudiosink";
  std::string device_;
  uint64_t beginning_offset_nanosec_ = 0;
  int64_t end_offset_nanosec_ = -1;
  unsigned volume_percent_ = 100;
  bool replaygain_enabled_ = false;
  bool fading_enabled_ = false;
  int fade_duration_ms_ = 2000;
  int fade_direction_ = 0;
  int fade_step_ = 0;
  int fade_steps_ = 1;
  guint fade_timeout_id_ = 0;
  std::vector<int16_t> last_scope_;
};

#endif  // STRAWBERRY_GSTENGINE_H
