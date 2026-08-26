#include "utilities/audioanalysis.h"

#include "utilities/fileutils.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace AudioAnalysis {

std::vector<float> PeaksFromPcm(const int16_t *samples, size_t count, size_t channels, size_t bins) {
  if (bins == 0) {
    return {};
  }
  std::vector<float> peaks(bins, 0.0f);
  if (!samples || count == 0) {
    return peaks;
  }
  const size_t ch = std::max<size_t>(1, channels);
  const size_t frames = count / ch;
  const size_t chunk = std::max<size_t>(1, frames / bins);
  for (size_t i = 0; i < bins; ++i) {
    float peak = 0.0f;
    for (size_t j = 0; j < chunk; ++j) {
      const size_t frame = i * chunk + j;
      if (frame >= frames) {
        break;
      }
      for (size_t c = 0; c < ch; ++c) {
        peak = std::max(peak, std::fabs(static_cast<float>(samples[frame * ch + c])) / 32768.0f);
      }
    }
    peaks[i] = peak;
  }
  return peaks;
}

std::vector<uint8_t> MoodFromPeaks(const std::vector<float> &peaks) {
  std::vector<uint8_t> out(peaks.size() * 3);
  for (size_t i = 0; i < peaks.size(); ++i) {
    const float p = std::clamp(peaks[i], 0.0f, 1.0f);
    out[i * 3 + 0] = static_cast<uint8_t>(p * 255.0f);
    out[i * 3 + 1] = static_cast<uint8_t>(std::sqrt(p) * 180.0f);
    out[i * 3 + 2] = static_cast<uint8_t>((1.0f - p) * 80.0f + 40.0f);
  }
  return out;
}

std::vector<int16_t> ScopeFromMagnitudes(const std::vector<float> &db) {
  std::vector<int16_t> scope(db.size());
  for (size_t i = 0; i < db.size(); ++i) {
    const float norm = std::clamp((db[i] + 80.0f) / 80.0f, 0.0f, 1.0f);
    scope[i] = static_cast<int16_t>(norm * 32767.0f);
  }
  return scope;
}

std::vector<int16_t> DecodePcm(const std::string &url, size_t max_samples) {
  if (url.empty() || max_samples == 0) {
    return {};
  }
  if (!gst_is_initialized()) {
    GError *error = nullptr;
    if (!gst_init_check(nullptr, nullptr, &error)) {
      if (error) {
        g_error_free(error);
      }
      return {};
    }
  }

  std::string uri = url;
  if (uri.find("://") == std::string::npos) {
    uri = FileUtils::UriFromPath(url);
  }

  GstElement *pipeline = gst_pipeline_new("decode-pcm");
  GstElement *src = gst_element_factory_make("uridecodebin", "src");
  GstElement *convert = gst_element_factory_make("audioconvert", "convert");
  GstElement *resample = gst_element_factory_make("audioresample", "resample");
  GstElement *sink = gst_element_factory_make("appsink", "sink");
  if (!pipeline || !src || !convert || !resample || !sink) {
    if (pipeline) {
      gst_object_unref(pipeline);
    }
    return {};
  }

  gst_bin_add_many(GST_BIN(pipeline), src, convert, resample, sink, nullptr);
  gst_element_link_many(convert, resample, sink, nullptr);
  GstCaps *caps = gst_caps_from_string("audio/x-raw,format=S16LE,channels=1,rate=11025");
  gst_app_sink_set_caps(GST_APP_SINK(sink), caps);
  gst_caps_unref(caps);
  gst_app_sink_set_max_buffers(GST_APP_SINK(sink), 8);
  g_object_set(sink, "sync", FALSE, nullptr);
  g_object_set(src, "uri", uri.c_str(), nullptr);
  g_signal_connect(src, "pad-added", G_CALLBACK(+[](GstElement *, GstPad *pad, gpointer data) {
                     GstPad *sinkpad = gst_element_get_static_pad(static_cast<GstElement *>(data), "sink");
                     if (sinkpad && !gst_pad_is_linked(sinkpad)) {
                       gst_pad_link(pad, sinkpad);
                     }
                     if (sinkpad) {
                       gst_object_unref(sinkpad);
                     }
                   }),
                   convert);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  std::vector<int16_t> pcm;
  pcm.reserve(std::min(max_samples, static_cast<size_t>(11025 * 8)));
  while (pcm.size() < max_samples) {
    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), GST_SECOND);
    if (!sample) {
      break;
    }
    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    if (buf && gst_buffer_map(buf, &map, GST_MAP_READ)) {
      const size_t n = map.size / sizeof(int16_t);
      const auto *samples = reinterpret_cast<const int16_t *>(map.data);
      const size_t take = std::min(n, max_samples - pcm.size());
      pcm.insert(pcm.end(), samples, samples + take);
      gst_buffer_unmap(buf, &map);
    }
    gst_sample_unref(sample);
  }
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  return pcm;
}

}  // namespace AudioAnalysis
