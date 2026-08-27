#include "engine/gstengine.h"

#include "core/taskmanager.h"
#include "engine/enginebuffering.h"
#include "constants/backendsettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "core/networkproxyfactory.h"
#include "engine/backendoptions.h"
#include "equalizer/equalizerpersist.h"

#include <cstdlib>

#include <algorithm>

GstEngine::GstEngine() = default;

GstEngine::~GstEngine() {
  CancelFade();
  DiscardNext();
  BufferingFinished();
  current_.reset();
}

bool GstEngine::Init() {
  GError *error = nullptr;
  if (!gst_is_initialized() && !gst_init_check(nullptr, nullptr, &error)) {
    if (error) {
      LogError("GStreamer init failed: %s", error->message);
      g_error_free(error);
    }
    return false;
  }

  Settings settings;
  settings.BeginGroup("Backend");
  output_ = settings.Value("output", DefaultOutput());
  device_ = settings.Value("device");
  replaygain_enabled_ = settings.BoolValue("rgenabled", false);
  if (settings.Contains("rgmode")) {
    const std::string mode = settings.Value("rgmode");
    replaygain_mode_ = (mode == "track" || mode == "1") ? 1 : settings.IntValue("rgmode", 0);
  }
  replaygain_preamp_ = settings.DoubleValue("rgpreamp", 0);
  Settings eq;
  eq.BeginGroup(EqualizerPersist::kSettingsGroup);
  if (eq.Contains(EqualizerPersist::kEnableStereoBalancer) || eq.Contains(EqualizerPersist::kStereoBalance)) {
    stereo_balance_ = EqualizerPersist::EffectiveBalanceFraction(eq.BoolValue(EqualizerPersist::kEnableStereoBalancer, false),
                                                                 eq.IntValue(EqualizerPersist::kStereoBalance, 0));
  } else {
    stereo_balance_ = static_cast<float>(settings.IntValue("stereobalance", 0)) / 100.0f;
  }
  eq_enabled_ = eq.BoolValue("enabled", false);
  fading_enabled_ = settings.Contains("FadeoutEnabled") ? settings.BoolValue("FadeoutEnabled") : settings.BoolValue("fading", false);
  autocrossfade_enabled_ = settings.Contains("AutoCrossfadeEnabled") ? settings.BoolValue("AutoCrossfadeEnabled")
                                                                    : settings.BoolValue("autocrossfade", fading_enabled_);
  fade_duration_ms_ = std::max(100, settings.Contains("FadeoutDuration") ? settings.IntValue("FadeoutDuration", 2000)
                                                                        : settings.IntValue("fadeduration", 2000));
  playbin3_ = settings.BoolValue(BackendSettings::kPlaybin3, BackendSettings::kDefaultPlaybin3);
  no_crossfade_same_album_ = settings.BoolValue(BackendSettings::kNoCrossfadeSameAlbum, BackendSettings::kDefaultNoCrossfadeSameAlbum);
  fadeout_pause_enabled_ = settings.BoolValue(BackendSettings::kFadeoutPauseEnabled, BackendSettings::kDefaultFadeoutPauseEnabled);
  fadeout_pause_duration_ms_ = std::max(
      50, settings.IntValue(BackendSettings::kFadeoutPauseDuration, static_cast<int>(BackendSettings::kDefaultFadeoutPauseDuration)));
  ReloadBackendOptions();
  return true;
}

void GstEngine::ReloadBackendOptions() {
  Settings settings;
  settings.BeginGroup(BackendSettings::kSettingsGroup);
  exclusive_mode_ = settings.BoolValue(BackendSettings::kExclusiveMode, BackendSettings::kDefaultExclusiveMode);
  volume_control_ = settings.BoolValue(BackendSettings::kVolumeControl, BackendSettings::kDefaultVolumeControl);
  volume_exponential_ = settings.BoolValue(BackendSettings::kVolumeExponential, BackendSettings::kDefaultVolumeExponential);
  channels_enabled_ = settings.BoolValue(BackendSettings::kChannelsEnabled, BackendSettings::kDefaultChannelsEnabled);
  channels_ = settings.IntValue(BackendSettings::kChannels, BackendSettings::kDefaultChannels);
  bs2b_enabled_ = settings.BoolValue(BackendSettings::kBS2B, BackendSettings::kDefaultBS2B);
  http2_enabled_ = settings.BoolValue(BackendSettings::kHTTP2, BackendSettings::kDefaultHTTP2);
  strict_ssl_enabled_ = settings.BoolValue(BackendSettings::kStrictSSL, BackendSettings::kDefaultStrictSSL);
  buffer_duration_ms_ = settings.Int64Value(BackendSettings::kBufferDuration, BackendSettings::kDefaultBufferDuration);
  buffer_low_watermark_ = BackendOptions::ClampWatermark(
      settings.DoubleValue(BackendSettings::kBufferLowWatermark, BackendSettings::kDefaultBufferLowWatermark));
  buffer_high_watermark_ = BackendOptions::ClampWatermark(
      settings.DoubleValue(BackendSettings::kBufferHighWatermark, BackendSettings::kDefaultBufferHighWatermark));
  device_warmup_ms_ = settings.IntValue(BackendSettings::kDeviceWarmupDuration, BackendSettings::kDefaultDeviceWarmupDuration);
  replaygain_fallback_ = settings.DoubleValue(BackendSettings::kRgFallbackGain, BackendSettings::kDefaultRgFallbackGain);
  replaygain_compression_ = settings.BoolValue(BackendSettings::kRgCompression, BackendSettings::kDefaultRgCompression);
  setenv("SOUP_FORCE_HTTP1", BackendOptions::SoupForceHttp1(http2_enabled_), 1);
  NetworkProxyFactory proxy;
  proxy.ReloadSettings();
  const GstEngineProxy::Options engine_proxy = proxy.EngineOptions();
  proxy_address_ = engine_proxy.address;
  proxy_authentication_ = engine_proxy.authentication;
  proxy_user_ = engine_proxy.user;
  proxy_pass_ = engine_proxy.pass;
}

double GstEngine::VolumeFraction() const { return BackendOptions::VolumeFraction(volume_percent_, volume_exponential_); }

GstPipelineExtras GstEngine::PipelineExtras() const {
  GstPipelineExtras extras;
  extras.replaygain_fallback = replaygain_fallback_;
  extras.replaygain_compression = replaygain_compression_;
  extras.exclusive = exclusive_mode_;
  extras.volume_control = volume_control_;
  extras.volume_exponential = volume_exponential_;
  extras.channels_enabled = channels_enabled_;
  extras.channels = channels_;
  extras.bs2b = bs2b_enabled_;
  extras.strict_ssl = strict_ssl_enabled_;
  extras.proxy_address = proxy_address_;
  extras.proxy_authentication = proxy_authentication_;
  extras.proxy_user = proxy_user_;
  extras.proxy_pass = proxy_pass_;
  extras.buffer_duration_ms = buffer_duration_ms_;
  extras.buffer_low_watermark = buffer_low_watermark_;
  extras.buffer_high_watermark = buffer_high_watermark_;
  extras.device_warmup_ms = BackendOptions::WarmupMs(!current_, static_cast<int>(device_warmup_ms_));
  return extras;
}

std::unique_ptr<GstEnginePipeline> GstEngine::CreatePipeline(const std::string &url, uint64_t beginning_offset_nanosec,
                                                             int64_t end_offset_nanosec) {
  auto pipeline = std::make_unique<GstEnginePipeline>(next_pipeline_id_++);
  if (!pipeline->Create(url, output_, device_, beginning_offset_nanosec, end_offset_nanosec, replaygain_enabled_, replaygain_mode_,
                        replaygain_preamp_, stereo_balance_, playbin3_, PipelineExtras())) {
    return nullptr;
  }
  pipeline->SetEqualizer(eq_enabled_ ? eq_preamp_ : 0, eq_enabled_ ? eq_gains_ : std::vector<int>(10, 0));
  pipeline->SetVolume(VolumeFraction());
  WirePipeline(pipeline.get());
  return pipeline;
}

void GstEngine::WirePipeline(GstEnginePipeline *pipeline) {
  pipeline->AboutToFinish = [this](int id) { OnAboutToFinish(id); };
  pipeline->EosReached = [this](int id) { OnEos(id); };
  pipeline->StreamStarted = [this](int id) {
    if (current_ && current_->id() == id) {
      SetState(State::Playing);
    }
  };
  pipeline->ErrorOccurred = [this](int, const std::string &text) {
    SetState(State::Error);
    Error.Emit(text);
  };
  pipeline->SpectrumReady = [this](int id, const std::vector<int16_t> &scope) {
    if (current_ && current_->id() == id) {
      last_scope_ = scope;
      scope_ = last_scope_;
      ScopeUpdated.Emit(last_scope_);
    }
  };
  pipeline->TagsReady = [this, pipeline](int, const Song &song) {
    Song tagged = song;
    if (tagged.url().empty()) {
      tagged.set_url(pipeline->url());
    }
    MetadataReceived.Emit(tagged);
  };
  pipeline->Buffering = [this](int, int percent) { HandleBuffering(percent); };
}

void GstEngine::StartPreloading(const std::string &media_url, const std::string &stream_url, bool, int64_t beginning_offset_nanosec,
                                int64_t end_offset_nanosec) {
  const std::string url = stream_url.empty() ? media_url : stream_url;
  if (url.empty()) {
    return;
  }
  DiscardNext();
  next_url_ = url;
  next_ = CreatePipeline(url, static_cast<uint64_t>(std::max<int64_t>(0, beginning_offset_nanosec)), end_offset_nanosec);
  if (next_) {
    next_->SetVolume(0.0);
  }
}

bool GstEngine::Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool, uint64_t beginning_offset_nanosec,
                     int64_t end_offset_nanosec, std::optional<double>) {
  const std::string url = stream_url.empty() ? media_url : stream_url;
  const bool auto_change = (track_change_flags & Auto) != 0;
  const bool same_album = (track_change_flags & SameAlbum) != 0;
  const bool auto_crossfade = BackendOptions::AllowAutoCrossfade(autocrossfade_enabled_, no_crossfade_same_album_, current_album_,
                                                                next_album_, same_album) &&
                              !BackendOptions::SuppressSameAlbumCrossfade(auto_change, same_album, no_crossfade_same_album_);
  const bool crossfade = current_ && current_->valid() &&
                         ((fading_enabled_ && (track_change_flags & Manual)) || (auto_crossfade && auto_change) ||
                          ((fading_enabled_ || auto_crossfade) && (track_change_flags & Intro)));

  if (auto_change && current_ && current_->valid() && !crossfade) {
    DiscardNext();
    current_->SetNextUri(url);
    gapless_pending_ = true;
    return true;
  }

  if (crossfade) {
    if (next_ && next_->valid() && next_url_ == url) {
      return true;
    }
    DiscardNext();
    next_url_ = url;
    next_ = CreatePipeline(url, beginning_offset_nanosec, end_offset_nanosec);
    if (!next_) {
      Error.Emit("Could not create next playbin");
      return false;
    }
    next_->SetVolume(0.0);
    return true;
  }

  CancelFade();
  DiscardNext();
  current_.reset();
  current_ = CreatePipeline(url, beginning_offset_nanosec, end_offset_nanosec);
  if (!current_) {
    Error.Emit("Could not create playbin");
    return false;
  }
  gapless_pending_ = false;
  SetState(State::Idle);
  return true;
}

bool GstEngine::Play(bool pause, uint64_t offset_nanosec) {
  if (next_ && next_->valid() && current_ && current_->valid()) {
    if (!next_->Play(pause, offset_nanosec)) {
      Error.Emit("Failed to start next pipeline");
      DiscardNext();
      return false;
    }
    StartFade(1);
    SetState(pause ? State::Paused : State::Playing);
    return true;
  }
  if (!current_ || !current_->valid()) {
    return false;
  }
  if (gapless_pending_ && current_->is_playing()) {
    return true;
  }
  if (!current_->Play(pause, offset_nanosec)) {
    Error.Emit("Failed to start playback");
    SetState(State::Error);
    return false;
  }
  if (fading_enabled_ && !pause && !gapless_pending_) {
    ApplyCurrentVolume(0.0);
    StartFade(1);
  }
  SetState(pause ? State::Paused : State::Playing);
  return true;
}

void GstEngine::Stop(bool) {
  pending_pause_ = false;
  CancelFade();
  DiscardNext();
  BufferingFinished();
  if (current_) {
    current_->Stop();
  }
  current_.reset();
  gapless_pending_ = false;
  SetState(State::Empty);
}

void GstEngine::Pause() {
  if (!current_) {
    return;
  }
  if (fadeout_pause_enabled_) {
    pending_pause_ = true;
    StartFade(-1, fadeout_pause_duration_ms_);
    return;
  }
  current_->Pause();
  SetState(State::Paused);
}

void GstEngine::Unpause() {
  if (!current_) {
    return;
  }
  pending_pause_ = false;
  if (fadeout_pause_enabled_) {
    ApplyCurrentVolume(0.0);
    current_->Unpause();
    SetState(State::Playing);
    StartFade(1, fadeout_pause_duration_ms_);
    return;
  }
  current_->Unpause();
  SetState(State::Playing);
}

void GstEngine::Seek(uint64_t offset_nanosec) {
  if (current_) {
    current_->Seek(offset_nanosec);
  }
}

void GstEngine::ApplyCurrentVolume(double fraction) {
  if (current_) {
    current_->SetVolume(fraction);
  }
}

void GstEngine::SetVolumeSW(unsigned percent) {
  volume_percent_ = std::min(percent, 100u);
  if (fade_direction_ == 0) {
    ApplyCurrentVolume(VolumeFraction());
    if (next_) {
      next_->SetVolume(VolumeFraction());
    }
  }
}

void GstEngine::SetFadingEnabled(bool enabled) { fading_enabled_ = enabled; }

void GstEngine::SetAutoCrossfadeEnabled(bool enabled) { autocrossfade_enabled_ = enabled; }

void GstEngine::SetFadeDurationMs(int milliseconds) { fade_duration_ms_ = std::max(100, milliseconds); }

void GstEngine::SetFadeoutPauseDurationMs(int milliseconds) { fadeout_pause_duration_ms_ = std::max(50, milliseconds); }

void GstEngine::CancelFade() {
  if (fade_timeout_id_) {
    g_source_remove(fade_timeout_id_);
    fade_timeout_id_ = 0;
  }
  fade_direction_ = 0;
}

void GstEngine::StartFade(int direction, int duration_ms) {
  if (direction == 0) {
    return;
  }
  CancelFade();
  fade_direction_ = direction;
  fade_step_ = 0;
  const int duration = duration_ms > 0 ? duration_ms : fade_duration_ms_;
  fade_steps_ = std::max(1, duration / 50);
  fade_timeout_id_ = g_timeout_add(50, FadeTick, this);
}

gboolean GstEngine::FadeTick(gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  ++self->fade_step_;
  const double t = static_cast<double>(self->fade_step_) / static_cast<double>(self->fade_steps_);
  const double target = self->VolumeFraction();
  if (self->next_ && self->next_->valid()) {
    if (self->current_) {
      self->current_->SetVolume(target * std::max(0.0, 1.0 - t));
    }
    self->next_->SetVolume(target * std::min(1.0, t));
  } else if (self->fade_direction_ < 0) {
    self->ApplyCurrentVolume(target * std::max(0.0, 1.0 - t));
  } else {
    self->ApplyCurrentVolume(target * std::min(1.0, t));
  }
  if (self->fade_step_ >= self->fade_steps_) {
    self->fade_timeout_id_ = 0;
    self->fade_direction_ = 0;
    if (self->pending_pause_) {
      self->pending_pause_ = false;
      if (self->current_) {
        self->current_->Pause();
      }
      self->SetState(State::Paused);
    } else if (self->next_) {
      self->FinishCrossfade();
    }
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

void GstEngine::FinishCrossfade() {
  if (current_) {
    current_->Stop();
  }
  current_ = std::move(next_);
  if (current_) {
    current_->SetVolume(VolumeFraction());
  }
  gapless_pending_ = false;
}

void GstEngine::DiscardNext() {
  if (next_) {
    next_->Stop();
    next_.reset();
  }
  next_url_.clear();
}

void GstEngine::OnAboutToFinish(int pipeline_id) {
  if (!current_ || current_->id() != pipeline_id) {
    return;
  }
  if ((fading_enabled_ || autocrossfade_enabled_) && !next_) {
    StartFade(-1);
  }
  TrackAboutToEnd.Emit();
}

void GstEngine::OnEos(int pipeline_id) {
  if (next_ && current_ && current_->id() == pipeline_id) {
    CancelFade();
    FinishCrossfade();
    return;
  }
  if (current_ && current_->id() == pipeline_id) {
    if (gapless_pending_) {
      gapless_pending_ = false;
      return;
    }
    SetState(State::Idle);
    TrackEnded.Emit();
  }
}

int64_t GstEngine::position_nanosec() const { return current_ ? current_->position_nanosec() : 0; }

int64_t GstEngine::length_nanosec() const { return current_ ? current_->length_nanosec() : 0; }

std::vector<GstEngine::OutputDetails> GstEngine::GetOutputsList() const {
  return {
      {"autoaudiosink", "Automatic", "audio-card-symbolic"},
      {"pulsesink", "PulseAudio", "audio-card-symbolic"},
      {"pipewiresink", "PipeWire", "audio-card-symbolic"},
      {"alsasink", "ALSA", "audio-card-symbolic"},
  };
}

bool GstEngine::ValidOutput(const std::string &output) const { return gst_element_factory_find(output.c_str()) != nullptr; }

std::string GstEngine::DefaultOutput() const { return "autoaudiosink"; }

void GstEngine::SetOutput(const std::string &output, const std::string &device) {
  output_ = output.empty() ? DefaultOutput() : output;
  device_ = device;
}

void GstEngine::SetEqualizerEnabled(bool enabled) {
  eq_enabled_ = enabled;
  const int preamp = eq_enabled_ ? eq_preamp_ : 0;
  const std::vector<int> gains = eq_enabled_ ? eq_gains_ : std::vector<int>(10, 0);
  if (current_) {
    current_->SetEqualizer(preamp, gains);
  }
  if (next_) {
    next_->SetEqualizer(preamp, gains);
  }
}

void GstEngine::SetEqualizerParameters(int preamp, const std::vector<int> &band_gains) {
  eq_preamp_ = preamp;
  eq_gains_ = band_gains;
  const int applied_preamp = eq_enabled_ ? eq_preamp_ : 0;
  const std::vector<int> applied = eq_enabled_ ? eq_gains_ : std::vector<int>(10, 0);
  if (current_) {
    current_->SetEqualizer(applied_preamp, applied);
  }
  if (next_) {
    next_->SetEqualizer(applied_preamp, applied);
  }
}

void GstEngine::SetReplayGainEnabled(bool enabled) { replaygain_enabled_ = enabled; }
void GstEngine::SetReplayGainMode(int mode) { replaygain_mode_ = mode; }
void GstEngine::SetReplayGainPreamp(double preamp) { replaygain_preamp_ = preamp; }
void GstEngine::SetStereoBalance(float value) {
  stereo_balance_ = value;
  if (current_) {
    current_->SetStereoBalance(value);
  }
  if (next_) {
    next_->SetStereoBalance(value);
  }
}

void GstEngine::SetState(State state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  StateChanged.Emit(state);
}

void GstEngine::HandleBuffering(int percent) {
  if (EngineBuffering::ShouldStart(percent, buffering_task_id_ != -1)) {
    BufferingStarted();
  }
  if (buffering_task_id_ != -1) {
    BufferingProgress(percent);
  }
  if (EngineBuffering::ShouldFinish(percent, buffering_task_id_ != -1)) {
    BufferingFinished();
  }
}

void GstEngine::BufferingStarted() {
  if (!task_manager_) {
    return;
  }
  if (buffering_task_id_ != -1) {
    task_manager_->SetTaskFinished(buffering_task_id_);
  }
  buffering_task_id_ = task_manager_->StartTask(EngineBuffering::TaskName());
  task_manager_->SetTaskProgress(buffering_task_id_, 0, EngineBuffering::kProgressMax);
}

void GstEngine::BufferingProgress(int percent) {
  if (task_manager_ && buffering_task_id_ != -1) {
    task_manager_->SetTaskProgress(buffering_task_id_, percent, EngineBuffering::kProgressMax);
  }
}

void GstEngine::BufferingFinished() {
  if (task_manager_ && buffering_task_id_ != -1) {
    task_manager_->SetTaskFinished(buffering_task_id_);
  }
  buffering_task_id_ = -1;
}
