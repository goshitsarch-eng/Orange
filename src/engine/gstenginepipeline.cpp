#include "engine/gstenginepipeline.h"

#include "core/logging.h"
#include "engine/backendoptions.h"
#include "utilities/audioanalysis.h"

#include <algorithm>
#include <cstring>
#include <vector>

GstEnginePipeline::GstEnginePipeline(int id) : id_(id) {}

GstEnginePipeline::~GstEnginePipeline() {
  CancelWarmup();
  Stop();
  if (bus_watch_id_) {
    g_source_remove(bus_watch_id_);
    bus_watch_id_ = 0;
  }
  if (playbin_) {
    gst_object_unref(playbin_);
    playbin_ = nullptr;
    volume_ = nullptr;
    equalizer_ = nullptr;
    panorama_ = nullptr;
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

void GstEnginePipeline::CancelWarmup() {
  if (warmup_timeout_id_) {
    g_source_remove(warmup_timeout_id_);
    warmup_timeout_id_ = 0;
  }
}

bool GstEnginePipeline::Create(const std::string &url, const std::string &output, const std::string &device,
                               uint64_t beginning_offset_nanosec, int64_t end_offset_nanosec, bool replaygain, int replaygain_mode,
                               double replaygain_preamp, float stereo_balance, bool playbin3, const GstPipelineExtras &extras) {
  beginning_offset_nanosec_ = beginning_offset_nanosec;
  end_offset_nanosec_ = end_offset_nanosec;
  volume_control_ = extras.volume_control;
  strict_ssl_ = extras.strict_ssl;
  proxy_address_ = extras.proxy_address;
  proxy_authentication_ = extras.proxy_authentication;
  proxy_user_ = extras.proxy_user;
  proxy_pass_ = extras.proxy_pass;
  device_warmup_ms_ = extras.device_warmup_ms;
  url_ = url;
  if (playbin3) {
    playbin_ = gst_element_factory_make("playbin3", "playbin");
  }
  if (!playbin_) {
    playbin_ = gst_element_factory_make("playbin", "playbin");
  }
  if (!playbin_) {
    return false;
  }
  gst_object_ref_sink(playbin_);
  g_object_set(playbin_, "uri", url.c_str(), nullptr);

  GstElement *sink = MakeAudioSink(output, device);
  if (sink) {
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(sink), "exclusive")) {
      g_object_set(sink, "exclusive", extras.exclusive ? TRUE : FALSE, nullptr);
    }
    GstElement *bin = gst_bin_new("audio-bin");
    GstElement *queue = gst_element_factory_make("queue", "audioqueue");
    if (queue) {
      g_object_set(queue, "use-buffering", TRUE, nullptr);
      const int64_t buffer_ns = BackendOptions::BufferDurationNanosec(extras.buffer_duration_ms);
      if (buffer_ns > 0) {
        g_object_set(queue, "max-size-buffers", 0, "max-size-bytes", 0, "max-size-time", static_cast<guint64>(buffer_ns), nullptr);
      }
      g_object_set(queue, "low-watermark", BackendOptions::ClampWatermark(extras.buffer_low_watermark), "high-watermark",
                   BackendOptions::ClampWatermark(extras.buffer_high_watermark), nullptr);
    }
    volume_ = extras.volume_control ? gst_element_factory_make("volume", "volume") : nullptr;
    equalizer_ = gst_element_factory_make("equalizer-10bands", "equalizer");
    GstElement *rgvolume = replaygain ? gst_element_factory_make("rgvolume", "rgvolume") : nullptr;
    GstElement *rglimiter = replaygain ? gst_element_factory_make("rglimiter", "rglimiter") : nullptr;
    panorama_ = gst_element_factory_make("audiopanorama", "panorama");
    GstElement *panorama = panorama_;
    GstElement *bs2b = extras.bs2b ? gst_element_factory_make("bs2b", "bs2b") : nullptr;
    GstElement *spectrum = gst_element_factory_make("spectrum", "spectrum");
    if (spectrum) {
      g_object_set(spectrum, "bands", 64, "threshold", -80, "interval", GST_SECOND / 10, "post-messages", TRUE, "message-phase", FALSE,
                   nullptr);
    }
    GstElement *capsfilter = nullptr;
    const int channels = BackendOptions::EffectiveChannels(extras.channels_enabled, extras.channels);
    if (channels > 0) {
      capsfilter = gst_element_factory_make("capsfilter", "channels");
      if (capsfilter) {
        GstCaps *caps = gst_caps_new_simple("audio/x-raw", "channels", G_TYPE_INT, channels, nullptr);
        g_object_set(capsfilter, "caps", caps, nullptr);
        gst_caps_unref(caps);
      }
    }
    std::vector<GstElement *> chain;
    auto add = [&](GstElement *element) {
      if (element) {
        gst_bin_add(GST_BIN(bin), element);
        chain.push_back(element);
      }
    };
    add(queue);
    add(volume_);
    add(equalizer_);
    add(rgvolume);
    add(rglimiter);
    add(panorama);
    add(bs2b);
    add(spectrum);
    add(capsfilter);
    add(sink);
    for (size_t i = 0; i + 1 < chain.size(); ++i) {
      gst_element_link(chain[i], chain[i + 1]);
    }
    GstElement *head = chain.empty() ? sink : chain.front();
    if (rgvolume) {
      g_object_set(rgvolume, "album-mode", replaygain_mode == 0, "pre-amp", replaygain_preamp, "fallback-gain", extras.replaygain_fallback,
                   nullptr);
    }
    if (rglimiter) {
      g_object_set(rglimiter, "enabled", extras.replaygain_compression ? TRUE : FALSE, nullptr);
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
  g_signal_connect(playbin_, "source-setup", G_CALLBACK((+[](GstElement *, GstElement *source, gpointer data) {
                     auto *self = static_cast<GstEnginePipeline *>(data);
                     if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "ssl-strict")) {
                       g_object_set(source, "ssl-strict", self->strict_ssl_ ? TRUE : FALSE, nullptr);
                     }
                     if (!self->proxy_address_.empty() && g_object_class_find_property(G_OBJECT_GET_CLASS(source), "proxy")) {
                       g_object_set(source, "proxy", self->proxy_address_.c_str(), nullptr);
                       if (self->proxy_authentication_ && g_object_class_find_property(G_OBJECT_GET_CLASS(source), "proxy-id") &&
                           g_object_class_find_property(G_OBJECT_GET_CLASS(source), "proxy-pw")) {
                         g_object_set(source, "proxy-id", self->proxy_user_.c_str(), "proxy-pw", self->proxy_pass_.c_str(), nullptr);
                       }
                     }
                   })),
                   this);
  return true;
}

bool GstEnginePipeline::Play(bool pause, uint64_t offset_nanosec) {
  if (!playbin_) {
    return false;
  }
  CancelWarmup();
  const bool warmup = !pause && device_warmup_ms_ > 0;
  const GstState target = (pause || warmup) ? GST_STATE_PAUSED : GST_STATE_PLAYING;
  if (gst_element_set_state(playbin_, target) == GST_STATE_CHANGE_FAILURE) {
    return false;
  }
  if (offset_nanosec + beginning_offset_nanosec_ > 0) {
    Seek(offset_nanosec);
  }
  if (warmup) {
    warmup_timeout_id_ = g_timeout_add(device_warmup_ms_, [](gpointer data) -> gboolean {
      auto *self = static_cast<GstEnginePipeline *>(data);
      self->warmup_timeout_id_ = 0;
      if (self->playbin_) {
        gst_element_set_state(self->playbin_, GST_STATE_PLAYING);
      }
      return G_SOURCE_REMOVE;
    }, this);
  }
  return true;
}

void GstEnginePipeline::Stop() {
  CancelWarmup();
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
  if (!volume_control_) {
    return;
  }
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

void GstEnginePipeline::SetStereoBalance(float value) {
  if (panorama_) {
    g_object_set(panorama_, "panorama", value, nullptr);
  }
}

void GstEnginePipeline::SetNextUri(const std::string &url) {
  if (playbin_ && !url.empty()) {
    url_ = url;
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
  if (song.url().empty()) {
    song.set_url(url_);
  }
  gst_tag_list_unref(tags);
  if (TagsReady) {
    TagsReady(id_, song);
  }
}
