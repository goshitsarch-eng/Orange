#ifndef STRAWBERRY_GSTENGINE_H
#define STRAWBERRY_GSTENGINE_H

#include "core/signal.h"
#include "core/song.h"
#include "engine/enginebase.h"
#include "engine/gstenginepipeline.h"

#include <gst/gst.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class GstEngine : public EngineBase {
 public:
  using EngineBase::State;
  using EngineBase::TrackChangeType;
  using EngineBase::OutputDetails;

  GstEngine();
  ~GstEngine() override;

  bool Init() override;
  State state() const override { return state_; }

  void StartPreloading(const std::string &media_url, const std::string &stream_url, bool force_stop_at_end,
                       int64_t beginning_offset_nanosec, int64_t end_offset_nanosec) override;
  bool Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool force_stop_at_end,
            uint64_t beginning_offset_nanosec, int64_t end_offset_nanosec, std::optional<double> ebur128_lufs) override;
  bool Play(bool pause, uint64_t offset_nanosec) override;
  void Stop(bool stop_after = false) override;
  void Pause() override;
  void Unpause() override;
  void Seek(uint64_t offset_nanosec) override;
  void SetVolumeSW(unsigned percent) override;

  int64_t position_nanosec() const override;
  int64_t length_nanosec() const override;
  const Scope &scope() const override { return last_scope_; }

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
  void SetAutoCrossfadeEnabled(bool enabled);
  void SetFadeDurationMs(int milliseconds);
  void SetPlaybin3(bool enabled) { playbin3_ = enabled; }
  void SetNoCrossfadeSameAlbum(bool enabled) { no_crossfade_same_album_ = enabled; }
  void SetFadeoutPauseEnabled(bool enabled) { fadeout_pause_enabled_ = enabled; }
  void SetFadeoutPauseDurationMs(int milliseconds);
  void SetCurrentAlbum(const std::string &album) { current_album_ = album; }
  void SetNextAlbum(const std::string &album) { next_album_ = album; }
  bool fading_enabled() const { return fading_enabled_; }
  bool autocrossfade_enabled() const { return autocrossfade_enabled_; }
  bool playbin3() const { return playbin3_; }
  bool no_crossfade_same_album() const { return no_crossfade_same_album_; }
  bool fadeout_pause_enabled() const { return fadeout_pause_enabled_; }
  bool has_next_pipeline() const { return next_ != nullptr; }
  const std::vector<int16_t> &last_scope() const { return last_scope_; }

  Signal<std::vector<int16_t>> ScopeUpdated;

 private:
  std::unique_ptr<GstEnginePipeline> CreatePipeline(const std::string &url, uint64_t beginning_offset_nanosec,
                                                    int64_t end_offset_nanosec);
  void WirePipeline(GstEnginePipeline *pipeline);
  void FinishCrossfade();
  void DiscardNext();
  void OnAboutToFinish(int pipeline_id);
  void OnEos(int pipeline_id);
  void StartFade(int direction, int duration_ms = 0);
  void CancelFade();
  static gboolean FadeTick(gpointer data);
  void SetState(State state);
  void ApplyCurrentVolume(double fraction);

  std::unique_ptr<GstEnginePipeline> current_;
  std::unique_ptr<GstEnginePipeline> next_;
  int next_pipeline_id_ = 1;
  int replaygain_mode_ = 0;
  double replaygain_preamp_ = 0.0;
  float stereo_balance_ = 0.0f;
  int eq_preamp_ = 0;
  std::vector<int> eq_gains_ = std::vector<int>(10, 0);
  bool eq_enabled_ = true;
  State state_ = State::Empty;
  std::string output_ = "autoaudiosink";
  std::string device_;
  unsigned volume_percent_ = 100;
  bool replaygain_enabled_ = false;
  bool fading_enabled_ = false;
  bool autocrossfade_enabled_ = false;
  bool playbin3_ = true;
  bool no_crossfade_same_album_ = true;
  bool fadeout_pause_enabled_ = false;
  bool pending_pause_ = false;
  int fade_duration_ms_ = 2000;
  int fadeout_pause_duration_ms_ = 250;
  std::string current_album_;
  std::string next_album_;
  int fade_direction_ = 0;
  int fade_step_ = 0;
  int fade_steps_ = 1;
  guint fade_timeout_id_ = 0;
  bool gapless_pending_ = false;
  std::vector<int16_t> last_scope_;
};

#endif  // STRAWBERRY_GSTENGINE_H
