#include "engine/gstengine.h"

#include "core/enginemetadata.h"
#include "core/taskmanager.h"
#include "engine/enginebuffering.h"
#include "engine/engineplay.h"
#include "engine/engineexclusive.h"
#include "engine/enginefade.h"
#include "engine/engineseek.h"
#include "engine/ebur128normalization.h"
#include "engine/enginediscoverer.h"
#include "engine/gstengineerror.h"
#include "constants/backendsettings.h"
#include "constants/spotifysettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "core/networkproxyfactory.h"
#include "engine/backendoptions.h"
#include "equalizer/equalizerpersist.h"

#include <gst/pbutils/pbutils.h>

#include <cstdlib>

#include <algorithm>

GstEngine::GstEngine() = default;

GstEngine::~GstEngine() {
  CancelFade();
  CancelStopFade();
  CancelSeek();
  DiscardNext();
  BufferingFinished();
  if (fadeout_) {
    fadeout_->Stop();
    fadeout_.reset();
  }
  current_.reset();
  DestroyDiscoverer();
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
  crossfade_enabled_ = settings.Contains("CrossfadeEnabled") ? settings.BoolValue("CrossfadeEnabled")
                                                            : settings.BoolValue("crossfade", fading_enabled_);
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
  ebur128_loudness_normalization_ =
      settings.BoolValue(BackendSettings::kEBUR128LoudnessNormalization, BackendSettings::kDefaultEBUR128LoudnessNormalization);
  ebur128_target_level_lufs_ =
      settings.DoubleValue(BackendSettings::kEBUR128TargetLevelLUFS, BackendSettings::kDefaultEBUR128TargetLevelLUFS);
  setenv("SOUP_FORCE_HTTP1", BackendOptions::SoupForceHttp1(http2_enabled_), 1);
  NetworkProxyFactory proxy;
  proxy.ReloadSettings();
  const GstEngineProxy::Options engine_proxy = proxy.EngineOptions();
  proxy_address_ = engine_proxy.address;
  proxy_authentication_ = engine_proxy.authentication;
  proxy_user_ = engine_proxy.user;
  proxy_pass_ = engine_proxy.pass;
  ReloadSpotifyAccessToken();
}

void GstEngine::ReloadSpotifyAccessToken() {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  std::string token = settings.Value(SpotifySettings::kAccessToken);
  if (token.empty()) {
    token = settings.Value("token");
  }
  UpdateSpotifyAccessToken(token);
}

void GstEngine::SetSpotifyAccessToken() {
  if (current_) {
    current_->set_spotify_access_token(spotify_access_token_);
  }
  if (next_) {
    next_->set_spotify_access_token(spotify_access_token_);
  }
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
  extras.spotify_access_token = spotify_access_token_;
  extras.ebur128_loudness_normalization = ebur128_loudness_normalization_;
  extras.ebur128_gain_db = ebur128_loudness_normalizing_gain_db_;
  return extras;
}

std::unique_ptr<GstEnginePipeline> GstEngine::CreatePipeline(const std::string &url, uint64_t beginning_offset_nanosec,
                                                             int64_t end_offset_nanosec, double ebur128_gain_db) {
  GstPipelineExtras extras = PipelineExtras();
  extras.ebur128_gain_db = ebur128_gain_db;
  auto pipeline = std::make_unique<GstEnginePipeline>(next_pipeline_id_++);
  if (!pipeline->Create(url, output_, device_, beginning_offset_nanosec, end_offset_nanosec, replaygain_enabled_, replaygain_mode_,
                        replaygain_preamp_, stereo_balance_, playbin3_, extras)) {
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
  pipeline->ErrorOccurred = [this](int id, int domain, int code, const std::string &text) {
    HandlePipelineError(id, domain, code, text);
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
  const std::string url = EngineDiscoverer::PlayUrl(media_url, stream_url);
  if (url.empty()) {
    return;
  }
  DiscardNext();
  next_media_url_ = media_url;
  next_url_ = url;
  if (!EngineExclusive::AllowsSecondPipeline(exclusive_mode_) && current_ && current_->valid()) {
    current_->SetNextUri(url);
    gapless_pending_ = true;
    RequestDiscover(media_url, url);
    return;
  }
  next_ = CreatePipeline(url, static_cast<uint64_t>(std::max<int64_t>(0, beginning_offset_nanosec)), end_offset_nanosec);
  if (next_) {
    next_->SetVolume(0.0);
  }
  RequestDiscover(media_url, url);
}

bool GstEngine::Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool, uint64_t beginning_offset_nanosec,
                     int64_t end_offset_nanosec, std::optional<double> ebur128_lufs) {
  const std::string url = EngineDiscoverer::PlayUrl(media_url, stream_url);
  ebur128_loudness_normalizing_gain_db_ =
      Ebur128Normalization::EffectiveGainDb(ebur128_loudness_normalization_, ebur128_lufs, ebur128_target_level_lufs_);
  const bool auto_change = (track_change_flags & Auto) != 0;
  const bool same_album = (track_change_flags & SameAlbum) != 0;
  const bool auto_crossfade = BackendOptions::AllowAutoCrossfade(autocrossfade_enabled_, no_crossfade_same_album_, current_album_,
                                                                next_album_, same_album) &&
                              !BackendOptions::SuppressSameAlbumCrossfade(auto_change, same_album, no_crossfade_same_album_);
  const bool want_crossfade = current_ && current_->valid() &&
                              ((crossfade_enabled_ && (track_change_flags & Manual)) || (auto_crossfade && auto_change) ||
                               ((crossfade_enabled_ || auto_crossfade) && (track_change_flags & Intro)));
  const bool crossfade = EngineExclusive::ShouldCrossfade(want_crossfade, exclusive_mode_);

  if (auto_change && current_ && current_->valid() && !crossfade) {
    DiscardNext();
    next_media_url_ = media_url;
    next_url_ = url;
    next_ebur128_gain_db_ = ebur128_loudness_normalizing_gain_db_;
    current_->SetNextUri(url);
    gapless_pending_ = true;
    RequestDiscover(media_url, url);
    return true;
  }

  if (crossfade) {
    if (next_ && next_->valid() && next_url_ == url) {
      next_->SetEbur128GainDb(ebur128_loudness_normalizing_gain_db_);
      return true;
    }
    DiscardNext();
    next_media_url_ = media_url;
    next_url_ = url;
    next_ = CreatePipeline(url, beginning_offset_nanosec, end_offset_nanosec, ebur128_loudness_normalizing_gain_db_);
    if (!next_) {
      Error.Emit("Could not create next playbin");
      return false;
    }
    next_->SetVolume(0.0);
    RequestDiscover(media_url, url);
    return true;
  }

  CancelFade();
  CancelSeek();
  DiscardNext();
  current_.reset();
  faded_out_to_pause_ = false;
  media_url_ = media_url;
  stream_url_ = url;
  current_ = CreatePipeline(url, beginning_offset_nanosec, end_offset_nanosec, ebur128_loudness_normalizing_gain_db_);
  if (!current_) {
    Error.Emit("Could not create playbin");
    SetState(State::Error);
    FatalError.Emit();
    return false;
  }
  gapless_pending_ = false;
  SetState(State::Idle);
  RequestDiscover(media_url, url);
  return true;
}

bool GstEngine::Play(bool pause, uint64_t offset_nanosec) {
  if (EngineExclusive::ShouldDelayPlay(exclusive_mode_, fadeout_ != nullptr)) {
    delayed_play_pending_ = true;
    delayed_play_pause_ = pause;
    delayed_play_offset_nanosec_ = offset_nanosec;
    return true;
  }
  delayed_play_pending_ = false;
  if (next_ && next_->valid() && current_ && current_->valid()) {
    if (!next_->Play(pause, offset_nanosec)) {
      Error.Emit("Failed to start next pipeline");
      DiscardNext();
      return false;
    }
    StartFade(1);
    SetState(pause ? State::Paused : State::Playing);
    ValidSongRequested.Emit(stream_url_);
    return true;
  }
  if (!current_ || !current_->valid()) {
    return false;
  }
  if (EnginePlay::IsBuffering(buffering_task_id_)) {
    return false;
  }
  if (gapless_pending_ && current_->is_playing()) {
    return true;
  }
  if (EnginePlay::ShouldShortCircuitPlayingPipeline(current_->is_playing(), buffering_task_id_)) {
    if (EnginePlay::ShouldSeekWhenAlreadyPlaying(offset_nanosec, current_->beginning_offset_nanosec())) {
      Seek(offset_nanosec);
      PlayDone(false);
    }
    return true;
  }
  if (!current_->Play(pause, offset_nanosec)) {
    // A GST_MESSAGE_ERROR is still on its way to HandlePipelineError.
    return false;
  }
  if (crossfade_enabled_ && !pause && !gapless_pending_) {
    ApplyCurrentVolume(0.0);
    StartFade(1);
  }
  SetState(pause ? State::Paused : State::Playing);
  ValidSongRequested.Emit(stream_url_);
  return true;
}

void GstEngine::Stop(bool stop_after) {
  pending_pause_ = false;
  delayed_play_pending_ = false;
  CancelFade();
  CancelSeek();
  DiscardNext();
  BufferingFinished();
  gapless_pending_ = false;
  const bool already_idle = state_ == State::Idle || state_ == State::Empty;
  if (EngineFade::ShouldFadeOnStop(fading_enabled_, stop_after, exclusive_mode_, already_idle) && current_ && current_->valid()) {
    CancelStopFade();
    if (fadeout_) {
      fadeout_->Stop();
      fadeout_.reset();
    }
    fadeout_ = std::move(current_);
    media_url_.clear();
    stream_url_.clear();
    faded_out_to_pause_ = false;
    SetState(State::Empty);
    StartStopFade();
    return;
  }
  FinishStopImmediate();
}

void GstEngine::Pause() {
  if (!current_) {
    return;
  }
  if (EngineFade::ShouldFadeOnPause(fadeout_pause_enabled_, exclusive_mode_)) {
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
  if (EngineFade::ShouldFadeInOnResume(faded_out_to_pause_, fadeout_pause_enabled_, exclusive_mode_)) {
    ApplyCurrentVolume(0.0);
    current_->Unpause();
    SetState(State::Playing);
    StartFade(1, fadeout_pause_duration_ms_);
    faded_out_to_pause_ = false;
    return;
  }
  current_->Unpause();
  SetState(State::Playing);
}

void GstEngine::Seek(uint64_t offset_nanosec) {
  if (!current_) {
    return;
  }
  pending_seek_nanosec_ = offset_nanosec;
  waiting_to_seek_ = true;
  if (EngineSeek::ShouldSeekImmediately(seek_timeout_id_ != 0)) {
    SeekNow();
    seek_timeout_id_ = g_timeout_add(EngineSeek::kDelayMs, SeekTimeout, this);
  }
}

void GstEngine::SeekNow() {
  if (!EngineSeek::ShouldApplyPending(waiting_to_seek_)) {
    return;
  }
  waiting_to_seek_ = false;
  if (current_) {
    current_->Seek(pending_seek_nanosec_);
  }
}

void GstEngine::PlayDone(bool pause) {
  SetState(pause ? State::Paused : State::Playing);
  ValidSongRequested.Emit(stream_url_);
}

void GstEngine::CancelSeek() {
  if (seek_timeout_id_ != 0) {
    g_source_remove(seek_timeout_id_);
    seek_timeout_id_ = 0;
  }
  waiting_to_seek_ = false;
}

gboolean GstEngine::SeekTimeout(gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  self->seek_timeout_id_ = 0;
  self->SeekNow();
  return G_SOURCE_REMOVE;
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

void GstEngine::SetCrossfadeEnabled(bool enabled) { crossfade_enabled_ = enabled; }

void GstEngine::SetAutoCrossfadeEnabled(bool enabled) { autocrossfade_enabled_ = enabled; }

void GstEngine::SetFadeDurationMs(int milliseconds) { fade_duration_ms_ = std::max(100, milliseconds); }

void GstEngine::SetFadeoutPauseDurationMs(int milliseconds) { fadeout_pause_duration_ms_ = std::max(50, milliseconds); }

void GstEngine::CancelFade() {
  if (EngineFade::ShouldMarkFadedOutToPause(pending_pause_)) {
    faded_out_to_pause_ = true;
  }
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
      self->current_->SetVolume(EngineFade::VolumeAtStep(target, -1, t));
    }
    self->next_->SetVolume(EngineFade::VolumeAtStep(target, 1, t));
  } else {
    self->ApplyCurrentVolume(EngineFade::VolumeAtStep(target, self->fade_direction_, t));
  }
  if (self->fade_step_ >= self->fade_steps_) {
    self->fade_timeout_id_ = 0;
    self->fade_direction_ = 0;
    if (self->pending_pause_) {
      self->pending_pause_ = false;
      self->faded_out_to_pause_ = true;
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

void GstEngine::StartStopFade() {
  CancelStopFade();
  fadeout_step_ = 0;
  fadeout_steps_ = std::max(1, fade_duration_ms_ / 50);
  fadeout_timeout_id_ = g_timeout_add(50, StopFadeTick, this);
}

void GstEngine::CancelStopFade() {
  if (fadeout_timeout_id_) {
    g_source_remove(fadeout_timeout_id_);
    fadeout_timeout_id_ = 0;
  }
}

gboolean GstEngine::StopFadeTick(gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  ++self->fadeout_step_;
  const double t = static_cast<double>(self->fadeout_step_) / static_cast<double>(self->fadeout_steps_);
  if (self->fadeout_) {
    self->fadeout_->SetVolume(EngineFade::VolumeAtStep(self->VolumeFraction(), -1, t));
  }
  if (self->fadeout_step_ >= self->fadeout_steps_ || !self->fadeout_) {
    self->fadeout_timeout_id_ = 0;
    if (self->fadeout_) {
      self->fadeout_->Stop();
      self->fadeout_.reset();
    }
    if (self->delayed_play_pending_) {
      const bool pause = self->delayed_play_pause_;
      const uint64_t offset = self->delayed_play_offset_nanosec_;
      self->delayed_play_pending_ = false;
      self->Play(pause, offset);
    }
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

void GstEngine::FinishStopImmediate() {
  CancelSeek();
  CancelStopFade();
  if (fadeout_) {
    fadeout_->Stop();
    fadeout_.reset();
  }
  if (current_) {
    current_->Stop();
  }
  current_.reset();
  media_url_.clear();
  stream_url_.clear();
  gapless_pending_ = false;
  faded_out_to_pause_ = false;
  delayed_play_pending_ = false;
  SetState(State::Empty);
}

void GstEngine::FinishCrossfade() {
  if (current_) {
    current_->Stop();
  }
  current_ = std::move(next_);
  media_url_ = std::move(next_media_url_);
  stream_url_ = next_url_;
  next_media_url_.clear();
  next_url_.clear();
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
  next_media_url_.clear();
  next_url_.clear();
}

void GstEngine::OnAboutToFinish(int pipeline_id) {
  if (!current_ || current_->id() != pipeline_id) {
    return;
  }
  if ((crossfade_enabled_ || autocrossfade_enabled_) && !next_ && EngineExclusive::AllowsSecondPipeline(exclusive_mode_)) {
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
      media_url_ = std::move(next_media_url_);
      stream_url_ = next_url_;
      next_media_url_.clear();
      next_url_.clear();
      ebur128_loudness_normalizing_gain_db_ = next_ebur128_gain_db_;
      if (current_) {
        current_->SetEbur128GainDb(next_ebur128_gain_db_);
      }
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

void GstEngine::HandlePipelineError(int pipeline_id, int domain, int code, const std::string &text) {
  Error.Emit(text);
  if (GstEngineError::ShouldTearDownCurrent(current_ != nullptr, current_ ? current_->id() : 0, pipeline_id)) {
    const std::string url = stream_url_;
    CancelFade();
    current_.reset();
    DiscardNext();
    gapless_pending_ = false;
    BufferingFinished();
    SetState(State::Error);
    if (GstEngineError::IsInvalidSongError(domain, code)) {
      InvalidSongRequested.Emit(url);
    } else {
      FatalError.Emit();
    }
    return;
  }
  if (next_ && next_->id() == pipeline_id) {
    DiscardNext();
  }
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

void GstEngine::EnsureDiscoverer() {
  if (discoverer_) {
    return;
  }
  GError *error = nullptr;
  discoverer_ = gst_discoverer_new(static_cast<GstClockTime>(EngineDiscoverer::kDiscoveryTimeoutS) * GST_SECOND, &error);
  if (!discoverer_) {
    if (error) {
      LogError("Failed to create stream discoverer: %s", error->message);
      g_error_free(error);
    }
    return;
  }
  discovered_handler_ = g_signal_connect(
      discoverer_, "discovered",
      G_CALLBACK((+[](GstDiscoverer *, GstDiscovererInfo *info, GError *error, gpointer self) {
        static_cast<GstEngine *>(self)->OnStreamDiscovered(info, error);
      })),
      this);
  finished_handler_ = g_signal_connect(discoverer_, "finished", G_CALLBACK((+[](GstDiscoverer *, gpointer) {})), this);
  gst_discoverer_start(discoverer_);
}

void GstEngine::DestroyDiscoverer() {
  if (!discoverer_) {
    return;
  }
  if (discovered_handler_ != 0) {
    g_signal_handler_disconnect(discoverer_, discovered_handler_);
    discovered_handler_ = 0;
  }
  if (finished_handler_ != 0) {
    g_signal_handler_disconnect(discoverer_, finished_handler_);
    finished_handler_ = 0;
  }
  gst_discoverer_stop(discoverer_);
  g_object_unref(discoverer_);
  discoverer_ = nullptr;
}

void GstEngine::RequestDiscover(const std::string &media_url, const std::string &play_url) {
  if (!EngineDiscoverer::ShouldDiscover(media_url) || play_url.empty()) {
    return;
  }
  EnsureDiscoverer();
  if (!discoverer_) {
    return;
  }
  if (!gst_discoverer_discover_uri_async(discoverer_, play_url.c_str())) {
    LogError("Failed to start stream discovery for %s", play_url.c_str());
  }
}

void GstEngine::OnStreamDiscovered(GstDiscovererInfo *info, GError *) {
  if (!current_ || !info) {
    return;
  }
  const char *uri = gst_discoverer_info_get_uri(info);
  const std::string discovered = uri ? uri : "";
  const GstDiscovererResult result = gst_discoverer_info_get_result(info);
  if (result != GST_DISCOVERER_OK) {
    LogError("Stream discovery for %s failed: %s", discovered.c_str(), EngineDiscoverer::ErrorMessage(static_cast<int>(result)));
    return;
  }

  GList *audio_streams = gst_discoverer_info_get_audio_streams(info);
  if (!audio_streams) {
    LogError("Could not detect an audio stream in %s", discovered.c_str());
    return;
  }

  GstDiscovererStreamInfo *stream_info = reinterpret_cast<GstDiscovererStreamInfo *>(g_list_first(audio_streams)->data);
  EngineMetadata engine_metadata;
  engine_metadata.type = EngineDiscoverer::MatchType(discovered, current_ ? current_->url() : std::string(),
                                                    next_ ? next_->url() : next_url_);
  if (engine_metadata.type == EngineMetadata::Type::Current) {
    engine_metadata.media_url = media_url_;
    engine_metadata.stream_url = stream_url_;
  } else if (engine_metadata.type == EngineMetadata::Type::Next) {
    engine_metadata.media_url = next_media_url_;
    engine_metadata.stream_url = next_url_;
  }

  GstDiscovererAudioInfo *audio = GST_DISCOVERER_AUDIO_INFO(stream_info);
  engine_metadata.samplerate = static_cast<int>(gst_discoverer_audio_info_get_sample_rate(audio));
  engine_metadata.bitdepth = static_cast<int>(gst_discoverer_audio_info_get_depth(audio));
  engine_metadata.bitrate = static_cast<int>(gst_discoverer_audio_info_get_bitrate(audio) / 1000);

  GstCaps *caps = gst_discoverer_stream_info_get_caps(stream_info);
  if (caps) {
    const guint caps_size = gst_caps_get_size(caps);
    for (guint i = 0; i < caps_size; ++i) {
      GstStructure *structure = gst_caps_get_structure(caps, i);
      if (!structure) {
        continue;
      }
      const char *name = gst_structure_get_name(structure);
      const Song::FileType from_mime = EngineDiscoverer::FiletypeFromCapsMimetype(name ? name : "");
      if (from_mime != Song::FileType::Unknown) {
        engine_metadata.filetype = from_mime;
      }
    }
    if (engine_metadata.filetype == Song::FileType::Unknown) {
      gchar *codec_description = gst_pb_utils_get_codec_description(caps);
      engine_metadata.filetype = EngineDiscoverer::FiletypeFromCodecDescription(codec_description ? codec_description : "");
      g_free(codec_description);
    }
    gst_caps_unref(caps);
  }

  gst_discoverer_stream_info_list_free(audio_streams);
  LogDebug("Got stream info for %s: %s", discovered.c_str(), Song::FiletypeToString(engine_metadata.filetype).c_str());
  MetadataReceived.Emit(engine_metadata.ToSong());
}
