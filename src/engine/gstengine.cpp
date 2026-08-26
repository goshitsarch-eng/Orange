#include "engine/gstengine.h"

#include "constants/backendsettings.h"
#include "core/logging.h"
#include "core/settings.h"
#include "engine/backendoptions.h"

#include <algorithm>

GstEngine::GstEngine() = default;

GstEngine::~GstEngine() {
  CancelFade();
  DiscardNext();
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
  replaygain_preamp_ = settings.IntValue("rgpreamp", 0);
  stereo_balance_ = static_cast<float>(settings.IntValue("stereobalance", 0)) / 100.0f;
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
  return true;
}

std::unique_ptr<GstEnginePipeline> GstEngine::CreatePipeline(const std::string &url, uint64_t beginning_offset_nanosec,
                                                             int64_t end_offset_nanosec) {
  auto pipeline = std::make_unique<GstEnginePipeline>(next_pipeline_id_++);
  if (!pipeline->Create(url, output_, device_, beginning_offset_nanosec, end_offset_nanosec, replaygain_enabled_, replaygain_mode_,
                        replaygain_preamp_, stereo_balance_, playbin3_)) {
    return nullptr;
  }
  pipeline->SetEqualizer(eq_preamp_, eq_gains_);
  pipeline->SetVolume(volume_percent_ / 100.0);
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
  pipeline->TagsReady = [this](int, const Song &song) { MetadataReceived.Emit(song); };
}

void GstEngine::StartPreloading(const std::string &media_url, const std::string &stream_url, bool, int64_t beginning_offset_nanosec,
                                int64_t end_offset_nanosec) {
  const std::string url = stream_url.empty() ? media_url : stream_url;
  if (url.empty()) {
    return;
  }
  DiscardNext();
  next_ = CreatePipeline(url, static_cast<uint64_t>(std::max<int64_t>(0, beginning_offset_nanosec)), end_offset_nanosec);
  if (next_) {
    next_->SetVolume(0.0);
  }
}

bool GstEngine::Load(const std::string &media_url, const std::string &stream_url, int track_change_flags, bool, uint64_t beginning_offset_nanosec,
                     int64_t end_offset_nanosec, std::optional<double>) {
  const std::string url = stream_url.empty() ? media_url : stream_url;
  const bool auto_change = (track_change_flags & Auto) != 0;
  const bool auto_crossfade = BackendOptions::AllowAutoCrossfade(autocrossfade_enabled_, no_crossfade_same_album_, current_album_, next_album_);
  const bool crossfade = current_ && current_->valid() &&
                         ((fading_enabled_ && (track_change_flags & Manual)) || (auto_crossfade && auto_change) ||
                          ((fading_enabled_ || auto_crossfade) && (track_change_flags & Intro)));

  if (auto_change && current_ && current_->valid() && !crossfade) {
    current_->SetNextUri(url);
    gapless_pending_ = true;
    return true;
  }

  if (crossfade) {
    DiscardNext();
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
    ApplyCurrentVolume(volume_percent_ / 100.0);
    if (next_) {
      next_->SetVolume(volume_percent_ / 100.0);
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
  const double target = self->volume_percent_ / 100.0;
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
    current_->SetVolume(volume_percent_ / 100.0);
  }
  gapless_pending_ = false;
}

void GstEngine::DiscardNext() {
  if (next_) {
    next_->Stop();
    next_.reset();
  }
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

void GstEngine::SetEqualizerEnabled(bool enabled) { eq_enabled_ = enabled; }

void GstEngine::SetEqualizerParameters(int preamp, const std::vector<int> &band_gains) {
  eq_preamp_ = preamp;
  eq_gains_ = band_gains;
  if (current_) {
    current_->SetEqualizer(preamp, band_gains);
  }
  if (next_) {
    next_->SetEqualizer(preamp, band_gains);
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
