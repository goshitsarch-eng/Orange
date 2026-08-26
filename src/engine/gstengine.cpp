#include "engine/gstengine.h"

#include "core/logging.h"
#include "core/settings.h"

#include <gst/audio/audio.h>
#include <gst/pbutils/pbutils.h>

#include <cmath>
#include <cstring>

GstEngine::GstEngine() = default;

GstEngine::~GstEngine() {
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
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
    playbin_ = nullptr;
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
    rgvolume_ = gst_element_factory_make("rgvolume", "rgvolume");
    if (volume_) gst_bin_add(GST_BIN(bin), volume_);
    if (equalizer_) gst_bin_add(GST_BIN(bin), equalizer_);
    if (rgvolume_ && replaygain_enabled_) gst_bin_add(GST_BIN(bin), rgvolume_);
    gst_bin_add(GST_BIN(bin), sink);

    GstElement *head = sink;
    if (volume_) {
      gst_element_link(volume_, equalizer_ ? equalizer_ : sink);
      head = volume_;
    }
    if (equalizer_ && volume_) {
      gst_element_link(equalizer_, sink);
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

void GstEngine::SetVolumeSW(unsigned percent) {
  volume_percent_ = std::min(percent, 100u);
  if (volume_) {
    g_object_set(volume_, "volume", volume_percent_ / 100.0, nullptr);
  } else if (playbin_) {
    g_object_set(playbin_, "volume", volume_percent_ / 100.0, nullptr);
  }
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
void GstEngine::SetReplayGainMode(int) {}
void GstEngine::SetReplayGainPreamp(double preamp) {
  if (rgvolume_) {
    g_object_set(rgvolume_, "preamp", preamp, nullptr);
  }
}
void GstEngine::SetStereoBalance(float) {}

gboolean GstEngine::BusCallback(GstBus *, GstMessage *message, gpointer data) {
  auto *self = static_cast<GstEngine *>(data);
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      self->SetState(State::Idle);
      self->TrackEnded.Emit();
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
  static_cast<GstEngine *>(data)->TrackAboutToEnd.Emit();
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
