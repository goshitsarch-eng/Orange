#include "engine/gstengine.h"

#include "core/logging.h"
#include "core/settings.h"
#include "utilities/audioanalysis.h"

#include <gst/audio/audio.h>
#include <gst/pbutils/pbutils.h>

#include <algorithm>
#include <cmath>
#include <cstring>

GstEngine::GstEngine() = default;

GstEngine::~GstEngine() {
  CancelFade();
  Stop();
  if (bus_watch_id_) {
    g_source_remove(bus_watch_id_);
  }
  if (pipeline_) {
    gst_object_unref(pipeline_);
  }
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
  replaygain_mode_ = settings.Value("rgmode", "album") == "track" ? 1 : 0;
  replaygain_preamp_ = settings.IntValue("rgpreamp", 0);
  stereo_balance_ = static_cast<float>(settings.IntValue("stereobalance", 0)) / 100.0f;
  fading_enabled_ = settings.BoolValue("fading", false);
  fade_duration_ms_ = std::max(100, settings.IntValue("fadeduration", 2000));
  return true;
}

GstElement *GstEngine::MakeAudioSink() const {
  GstElement *sink = gst_element_factory_make(output_.c_str(), "audiosink");
  if (!sink) {
    sink = gst_element_factory_make("autoaudiosink", "audiosink");
  }
  if (sink && !device_.empty() && g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "device")) {
    g_object_set(sink, "device", device_.c_str(), nullptr);
  }
  return sink;
}

bool GstEngine::Load(const std::string &media_url, const std::string &stream_url, int, bool, uint64_t beginning_offset_nanosec,
                     int64_t end_offset_nanosec, std::optional<double>) {
  beginning_offset_nanosec_ = beginning_offset_nanosec;
  end_offset_nanosec_ = end_offset_nanosec;

  if (pipeline_) {
    CancelFade();
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
    playbin_ = nullptr;
    volume_ = nullptr;
    equalizer_ = nullptr;
    rgvolume_ = nullptr;
    rglimiter_ = nullptr;
    panorama_ = nullptr;
    spectrum_ = nullptr;
  }

  playbin_ = gst_element_factory_make("playbin", "playbin");
  if (!playbin_) {
    Error.Emit("Could not create playbin");
    return false;
  }
  pipeline_ = playbin_;
  gst_object_ref_sink(pipeline_);

  const std::string url = stream_url.empty() ? media_url : stream_url;
  g_object_set(playbin_, "uri", url.c_str(), nullptr);

  GstElement *sink = MakeAudioSink();
  if (sink) {
    GstElement *bin = gst_bin_new("audio-bin");
    volume_ = gst_element_factory_make("volume", "volume");
    equalizer_ = gst_element_factory_make("equalizer-10bands", "equalizer");
    rgvolume_ = replaygain_enabled_ ? gst_element_factory_make("rgvolume", "rgvolume") : nullptr;
    rglimiter_ = replaygain_enabled_ ? gst_element_factory_make("rglimiter", "rglimiter") : nullptr;
    panorama_ = gst_element_factory_make("audiopanorama", "panorama");
    spectrum_ = gst_element_factory_make("spectrum", "spectrum");
    if (spectrum_) {
      g_object_set(spectrum_, "bands", 64, "threshold", -80, "interval", GST_SECOND / 10, "post-messages", TRUE, "message-phase", FALSE,
                   nullptr);
    }
    std::vector<GstElement *> chain;
    auto add = [&](GstElement *element) {
      if (element) {
        gst_bin_add(GST_BIN(bin), element);
        chain.push_back(element);
      }
    };
    add(volume_);
    add(equalizer_);
    add(rgvolume_);
    add(rglimiter_);
    add(panorama_);
    add(spectrum_);
    add(sink);
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
      gst_element_link(chain[i], chain[i + 1]);
    }
    GstElement *head = chain.empty() ? sink : chain.front();
    if (rgvolume_) {
      g_object_set(rgvolume_, "album-mode", replaygain_mode_ == 0, "pre-amp", replaygain_preamp_, nullptr);
    }
    if (panorama_) {
      g_object_set(panorama_, "panorama", stereo_balance_, nullptr);
    }
    GstPad *pad = gst_element_get_static_pad(head, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", pad);
    gst_element_add_pad(bin, ghost);
    gst_object_unref(pad);
    g_object_set(playbin_, "audio-sink", bin, nullptr);
  }

  GstBus *bus = gst_element_get_bus(pipeline_);
  if (bus_watch_id_) {
    g_source_remove(bus_watch_id_);
  }
  bus_watch_id_ = gst_bus_add_watch(bus, BusCallback, this);
  gst_object_unref(bus);
  g_signal_connect(playbin_, "about-to-finish", G_CALLBACK(AboutToFinish), this);
  SetVolumeSW(volume_percent_);
  SetState(State::Idle);
  return true;
}

bool GstEngine::Play(bool pause, uint64_t offset_nanosec) {
  if (!pipeline_) {
    return false;
  }
  const GstState target = pause ? GST_STATE_PAUSED : GST_STATE_PLAYING;
  if (gst_element_set_state(pipeline_, target) == GST_STATE_CHANGE_FAILURE) {
    Error.Emit("Failed to start playback");
    SetState(State::Error);
    return false;
  }
  if (offset_nanosec + beginning_offset_nanosec_ > 0) {
    Seek(offset_nanosec);
  }
  if (fading_enabled_ && !pause) {
    ApplyVolume(0.0);
    StartFade(1);
  }
  SetState(pause ? State::Paused : State::Playing);
  return true;
}

void GstEngine::Stop(bool) {
  if (!pipeline_) {
    SetState(State::Empty);
    return;
  }
  gst_element_set_state(pipeline_, GST_STATE_NULL);
  SetState(State::Empty);
}

void GstEngine::Pause() {
  if (pipeline_ && gst_element_set_state(pipeline_, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE) {
    SetState(State::Paused);
  }
}

void GstEngine::Unpause() {
  if (pipeline_ && gst_element_set_state(pipeline_, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE) {
    SetState(State::Playing);
  }
}

void GstEngine::Seek(uint64_t offset_nanosec) {
  if (!pipeline_) {
    return;
  }
  const gint64 position = static_cast<gint64>(beginning_offset_nanosec_ + offset_nanosec);
  gst_element_seek_simple(pipeline_, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                          position);
}

void GstEngine::ApplyVolume(double fraction) {
  fraction = std::clamp(fraction, 0.0, 1.0);
  if (volume_) {
    g_object_set(volume_, "volume", fraction, nullptr);
  } else if (playbin_) {
    g_object_set(playbin_, "volume", fraction, nullptr);
  }
}

void GstEngine::SetVolumeSW(unsigned percent) {
  volume_percent_ = std::min(percent, 100u);
  if (fade_direction_ == 0) {
    ApplyVolume(volume_percent_ / 100.0);
  }
}

void GstEngine::SetFadingEnabled(bool enabled) { fading_enabled_ = enabled; }

void GstEngine::SetFadeDurationMs(int milliseconds) { fade_duration_ms_ = std::max(100, milliseconds); }

void GstEngine::CancelFade() {
  if (fade_timeout_id_) {
    g_source_remove(fade_timeout_id_);
    fade_timeout_id_ = 0;
  }
  fade_direction_ = 0;
}

void GstEngine::StartFade(int direction) {
  if (!fading_enabled_ || direction == 0) {
    return;
  }
  CancelFade();
  fade_direction_ = direction;
  fade_step_ = 0;
  fade_steps_ = std::max(1, fade_duration_ms_ / 50);
  fade_timeout_id_ = g_timeout_add(50, FadeTick, this);
}

gboolean GstEngine::FadeTick(gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  ++self->fade_step_;
  const double t = static_cast<double>(self->fade_step_) / static_cast<double>(self->fade_steps_);
  const double target = self->volume_percent_ / 100.0;
  if (self->fade_direction_ < 0) {
    self->ApplyVolume(target * std::max(0.0, 1.0 - t));
  } else {
    self->ApplyVolume(target * std::min(1.0, t));
  }
  if (self->fade_step_ >= self->fade_steps_) {
    self->fade_timeout_id_ = 0;
    self->fade_direction_ = 0;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

int64_t GstEngine::position_nanosec() const {
  if (!pipeline_) {
    return 0;
  }
  gint64 position = 0;
  if (!gst_element_query_position(pipeline_, GST_FORMAT_TIME, &position)) {
    return 0;
  }
  return position - static_cast<int64_t>(beginning_offset_nanosec_);
}

int64_t GstEngine::length_nanosec() const {
  if (!pipeline_) {
    return 0;
  }
  gint64 duration = 0;
  if (!gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &duration)) {
    return 0;
  }
  if (end_offset_nanosec_ > 0) {
    return end_offset_nanosec_ - static_cast<int64_t>(beginning_offset_nanosec_);
  }
  return duration - static_cast<int64_t>(beginning_offset_nanosec_);
}

std::vector<GstEngine::OutputDetails> GstEngine::GetOutputsList() const {
  std::vector<OutputDetails> outputs = {
      {"autoaudiosink", "Automatic", "audio-card-symbolic"},
      {"pulsesink", "PulseAudio", "audio-card-symbolic"},
      {"pipewiresink", "PipeWire", "audio-card-symbolic"},
      {"alsasink", "ALSA", "audio-card-symbolic"},
#ifdef HAVE_PULSE
#endif
  };
  return outputs;
}

bool GstEngine::ValidOutput(const std::string &output) const { return gst_element_factory_find(output.c_str()) != nullptr; }

std::string GstEngine::DefaultOutput() const { return "autoaudiosink"; }

void GstEngine::SetOutput(const std::string &output, const std::string &device) {
  output_ = output.empty() ? DefaultOutput() : output;
  device_ = device;
}

void GstEngine::SetEqualizerEnabled(bool enabled) {
  if (equalizer_) {
    g_object_set(equalizer_, "band0", enabled ? 0.0 : 0.0, nullptr);
  }
}

void GstEngine::SetEqualizerParameters(int preamp, const std::vector<int> &band_gains) {
  if (!equalizer_) {
    return;
  }
  (void)preamp;
  for (size_t i = 0; i < band_gains.size() && i < 10; ++i) {
    gchar name[16];
    g_snprintf(name, sizeof(name), "band%zu", i);
    g_object_set(equalizer_, name, static_cast<gdouble>(band_gains[i]), nullptr);
  }
}

void GstEngine::SetReplayGainEnabled(bool enabled) { replaygain_enabled_ = enabled; }
void GstEngine::SetReplayGainMode(int mode) {
  replaygain_mode_ = mode;
  if (rgvolume_) {
    g_object_set(rgvolume_, "album-mode", mode == 0, nullptr);
  }
}
void GstEngine::SetReplayGainPreamp(double preamp) {
  replaygain_preamp_ = preamp;
  if (rgvolume_) {
    g_object_set(rgvolume_, "pre-amp", preamp, nullptr);
  }
}
void GstEngine::SetStereoBalance(float value) {
  stereo_balance_ = value;
  if (panorama_) {
    g_object_set(panorama_, "panorama", value, nullptr);
  }
}

gboolean GstEngine::BusCallback(GstBus *, GstMessage *message, gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      self->SetState(State::Idle);
      self->TrackEnded.Emit();
      break;
    case GST_MESSAGE_ELEMENT:
      self->HandleSpectrum(message);
      break;
    case GST_MESSAGE_ERROR:
      self->HandleError(message);
      break;
    case GST_MESSAGE_TAG: {
      GstTagList *tags = nullptr;
      gst_message_parse_tag(message, &tags);
      if (tags) {
        Song song;
        gchar *title = nullptr;
        if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &title)) {
          song.set_title(title);
          g_free(title);
        }
        gchar *artist = nullptr;
        if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &artist)) {
          song.set_artist(artist);
          g_free(artist);
        }
        song.set_valid(true);
        self->MetadataReceived.Emit(song);
        gst_tag_list_unref(tags);
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}

void GstEngine::AboutToFinish(GstElement *, gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  if (self->fading_enabled_) {
    self->StartFade(-1);
  }
  self->TrackAboutToEnd.Emit();
}

void GstEngine::HandleSpectrum(GstMessage *message) {
  const GstStructure *structure = gst_message_get_structure(message);
  if (!structure || !gst_structure_has_name(structure, "spectrum")) {
    return;
  }
  const GValue *magnitudes = gst_structure_get_value(structure, "magnitude");
  if (!magnitudes) {
    return;
  }
  const guint n = gst_value_list_get_size(magnitudes);
  std::vector<float> db(n);
  for (guint i = 0; i < n; ++i) {
    const GValue *mag = gst_value_list_get_value(magnitudes, i);
    db[i] = mag ? g_value_get_float(mag) : -80.0f;
  }
  last_scope_ = AudioAnalysis::ScopeFromMagnitudes(db);
  ScopeUpdated.Emit(last_scope_);
}

void GstEngine::HandleError(GstMessage *message) {
  GError *error = nullptr;
  gchar *debug = nullptr;
  gst_message_parse_error(message, &error, &debug);
  const std::string text = error ? error->message : "Unknown GStreamer error";
  LogError("GStreamer error: %s (%s)", text.c_str(), debug ? debug : "");
  if (error) g_error_free(error);
  g_free(debug);
  SetState(State::Error);
  Error.Emit(text);
}

void GstEngine::SetState(State state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  StateChanged.Emit(state);
}
