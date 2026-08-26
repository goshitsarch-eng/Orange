#include "engine/gstenginepipeline.h"

#include "core/logging.h"
#include "utilities/audioanalysis.h"

#include <algorithm>
#include <cstring>
#include <vector>

GstEnginePipeline::GstEnginePipeline(int id) : id_(id) {}

GstEnginePipeline::~GstEnginePipeline() {
  Stop();
  if (bus_watch_id_) {
    g_source_remove(bus_watch_id_);
    bus_watch_id_ = 0;
  }
  if (playbin_) {
    gst_object_unref(playbin_);
    playbin_ = nullptr;
  }
}

GstElement *GstEnginePipeline::MakeAudioSink(const std::string &output, const std::string &device) const {
  GstElement *sink = gst_element_factory_make(output.c_str(), "audiosink");
  if (!sink) {
    sink = gst_element_factory_make("autoaudiosink", "audiosink");
  }
  if (sink && !device.empty() && g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "device")) {
    g_object_set(sink, "device", device.c_str(), nullptr);
  }
  return sink;
}

bool GstEnginePipeline::Create(const std::string &url, const std::string &output, const std::string &device,
                               uint64_t beginning_offset_nanosec, int64_t end_offset_nanosec, bool replaygain, int replaygain_mode,
                               double replaygain_preamp, float stereo_balance) {
  beginning_offset_nanosec_ = beginning_offset_nanosec;
  end_offset_nanosec_ = end_offset_nanosec;
  playbin_ = gst_element_factory_make("playbin", "playbin");
  if (!playbin_) {
    return false;
  }
  gst_object_ref_sink(playbin_);
  g_object_set(playbin_, "uri", url.c_str(), nullptr);

  GstElement *sink = MakeAudioSink(output, device);
  if (sink) {
    GstElement *bin = gst_bin_new("audio-bin");
    volume_ = gst_element_factory_make("volume", "volume");
    equalizer_ = gst_element_factory_make("equalizer-10bands", "equalizer");
    GstElement *rgvolume = replaygain ? gst_element_factory_make("rgvolume", "rgvolume") : nullptr;
    GstElement *rglimiter = replaygain ? gst_element_factory_make("rglimiter", "rglimiter") : nullptr;
    GstElement *panorama = gst_element_factory_make("audiopanorama", "panorama");
    GstElement *spectrum = gst_element_factory_make("spectrum", "spectrum");
    if (spectrum) {
      g_object_set(spectrum, "bands", 64, "threshold", -80, "interval", GST_SECOND / 10, "post-messages", TRUE, "message-phase", FALSE,
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
    add(rgvolume);
    add(rglimiter);
    add(panorama);
    add(spectrum);
    add(sink);
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
      gst_element_link(chain[i], chain[i + 1]);
    }
    GstElement *head = chain.empty() ? sink : chain.front();
    if (rgvolume) {
      g_object_set(rgvolume, "album-mode", replaygain_mode == 0, "pre-amp", replaygain_preamp, nullptr);
    }
    if (panorama) {
      g_object_set(panorama, "panorama", stereo_balance, nullptr);
    }
    GstPad *pad = gst_element_get_static_pad(head, "sink");
    GstPad *ghost = gst_ghost_pad_new("sink", pad);
    gst_element_add_pad(bin, ghost);
    gst_object_unref(pad);
    g_object_set(playbin_, "audio-sink", bin, nullptr);
  }

  GstBus *bus = gst_element_get_bus(playbin_);
  bus_watch_id_ = gst_bus_add_watch(bus, BusCallback, this);
  gst_object_unref(bus);
  g_signal_connect(playbin_, "about-to-finish", G_CALLBACK(AboutToFinishCb), this);
  return true;
}

bool GstEnginePipeline::Play(bool pause, uint64_t offset_nanosec) {
  if (!playbin_) {
    return false;
  }
  const GstState target = pause ? GST_STATE_PAUSED : GST_STATE_PLAYING;
  if (gst_element_set_state(playbin_, target) == GST_STATE_CHANGE_FAILURE) {
    return false;
  }
  if (offset_nanosec + beginning_offset_nanosec_ > 0) {
    Seek(offset_nanosec);
  }
  return true;
}

void GstEnginePipeline::Stop() {
  if (playbin_) {
    gst_element_set_state(playbin_, GST_STATE_NULL);
  }
}

void GstEnginePipeline::Pause() {
  if (playbin_) {
    gst_element_set_state(playbin_, GST_STATE_PAUSED);
  }
}

void GstEnginePipeline::Unpause() {
  if (playbin_) {
    gst_element_set_state(playbin_, GST_STATE_PLAYING);
  }
}

void GstEnginePipeline::Seek(uint64_t offset_nanosec) {
  if (!playbin_) {
    return;
  }
  gst_element_seek_simple(playbin_, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                          static_cast<gint64>(beginning_offset_nanosec_ + offset_nanosec));
}

void GstEnginePipeline::SetVolume(double fraction) {
  fraction = std::clamp(fraction, 0.0, 1.0);
  if (volume_) {
    g_object_set(volume_, "volume", fraction, nullptr);
  } else if (playbin_) {
    g_object_set(playbin_, "volume", fraction, nullptr);
  }
}

void GstEnginePipeline::SetEqualizer(int preamp, const std::vector<int> &band_gains) {
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

void GstEnginePipeline::SetNextUri(const std::string &url) {
  if (playbin_ && !url.empty()) {
    g_object_set(playbin_, "uri", url.c_str(), nullptr);
  }
}

int64_t GstEnginePipeline::position_nanosec() const {
  if (!playbin_) {
    return 0;
  }
  gint64 position = 0;
  if (!gst_element_query_position(playbin_, GST_FORMAT_TIME, &position)) {
    return 0;
  }
  return position - static_cast<int64_t>(beginning_offset_nanosec_);
}

int64_t GstEnginePipeline::length_nanosec() const {
  if (!playbin_) {
    return 0;
  }
  gint64 duration = 0;
  if (!gst_element_query_duration(playbin_, GST_FORMAT_TIME, &duration)) {
    return 0;
  }
  if (end_offset_nanosec_ > 0) {
    return end_offset_nanosec_ - static_cast<int64_t>(beginning_offset_nanosec_);
  }
  return duration - static_cast<int64_t>(beginning_offset_nanosec_);
}

bool GstEnginePipeline::is_playing() const {
  if (!playbin_) {
    return false;
  }
  GstState state = GST_STATE_NULL;
  gst_element_get_state(playbin_, &state, nullptr, 0);
  return state == GST_STATE_PLAYING;
}

void GstEnginePipeline::AboutToFinishCb(GstElement *, gpointer data) {
  auto *self = static_cast<GstEnginePipeline *>(data);
  if (self->AboutToFinish) {
    self->AboutToFinish(self->id_);
  }
}

gboolean GstEnginePipeline::BusCallback(GstBus *, GstMessage *message, gpointer data) {
  auto *self = static_cast<GstEnginePipeline *>(data);
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
      if (self->EosReached) {
        self->EosReached(self->id_);
      }
      break;
    case GST_MESSAGE_STREAM_START:
      if (self->StreamStarted) {
        self->StreamStarted(self->id_);
      }
      break;
    case GST_MESSAGE_ELEMENT:
      self->HandleSpectrum(message);
      break;
    case GST_MESSAGE_ERROR:
      self->HandleError(message);
      break;
    case GST_MESSAGE_TAG:
      self->HandleTags(message);
      break;
    default:
      break;
  }
  return TRUE;
}

void GstEnginePipeline::HandleSpectrum(GstMessage *message) {
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
  if (SpectrumReady) {
    SpectrumReady(id_, AudioAnalysis::ScopeFromMagnitudes(db));
  }
}

void GstEnginePipeline::HandleError(GstMessage *message) {
  GError *error = nullptr;
  gchar *debug = nullptr;
  gst_message_parse_error(message, &error, &debug);
  const std::string text = error ? error->message : "Unknown GStreamer error";
  LogError("GStreamer error: %s (%s)", text.c_str(), debug ? debug : "");
  if (error) {
    g_error_free(error);
  }
  g_free(debug);
  if (ErrorOccurred) {
    ErrorOccurred(id_, text);
  }
}

void GstEnginePipeline::HandleTags(GstMessage *message) {
  GstTagList *tags = nullptr;
  gst_message_parse_tag(message, &tags);
  if (!tags) {
    return;
  }
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
  gst_tag_list_unref(tags);
  if (TagsReady) {
    TagsReady(id_, song);
  }
}
