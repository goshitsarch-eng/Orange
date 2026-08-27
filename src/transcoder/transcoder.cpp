#include "transcoder/transcoder.h"

#include "core/logging.h"
#include "transcoder/transcodergstproperties.h"
#include "transcoder/transcoderoptionsdialog.h"
#include "transcoder/transcoderoptionsinterface.h"
#include "utilities/fileutils.h"

#include <gst/gst.h>

void Transcoder::AddJob(const Song &song, const std::string &destination, Format format) {
  jobs_.push_back({song, destination, format});
}

Transcoder::Preset Transcoder::PresetFor(Format format) {
  switch (format) {
    case Format::MP3:
      return {format, "MP3", "mp3", "audio/mpeg", {}};
    case Format::AAC:
      return {format, "AAC", "m4a", "audio/mpeg", "audio/mp4"};
    case Format::FLAC:
      return {format, "FLAC", "flac", "audio/x-flac", {}};
    case Format::OggVorbis:
      return {format, "Ogg Vorbis", "ogg", "audio/x-vorbis", "application/ogg"};
    case Format::Opus:
      return {format, "Opus", "opus", "audio/x-opus", "application/ogg"};
    case Format::Speex:
      return {format, "Speex", "spx", "audio/x-speex", "application/ogg"};
    case Format::WavPack:
      return {format, "WavPack", "wv", "audio/x-wavpack", {}};
    case Format::ASF:
      return {format, "ASF", "wma", "audio/x-wma", "video/x-ms-asf"};
  }
  return {Format::FLAC, "FLAC", "flac", "audio/x-flac", {}};
}

std::string Transcoder::PipelineFor(Format format, int quality) {
  auto options = TranscoderOptionsDialog::OptionsFor(format);
  options->Load();
  if (quality >= 0) {
    options->ApplyQuality(quality);
  }
  return options->PipelineFragment();
}

bool Transcoder::TranscodeFile(const Song &song, const std::string &destination, Format format) {
  const std::string src = FileUtils::PathFromUri(song.url());
  if (!FileUtils::IsFile(src)) {
    log_.push_back("Missing source: " + src);
    LogLine.Emit(log_.back());
    return false;
  }
  FileUtils::WriteFile(destination, {});
  const std::string fragment = PipelineFor(format, quality_);
  const std::string desc = "filesrc name=src ! decodebin ! audioconvert ! audioresample ! " + fragment + " ! filesink name=sink";
  GError *error = nullptr;
  GstElement *pipeline = gst_parse_launch(desc.c_str(), &error);
  if (!pipeline) {
    const std::string message = error ? error->message : "Could not create transcoder pipeline";
    if (error) {
      g_error_free(error);
    }
    log_.push_back(message + " (" + fragment + ")");
    LogLine.Emit(log_.back());
    FileUtils::Remove(destination);
    return false;
  }
  GstElement *src_el = gst_bin_get_by_name(GST_BIN(pipeline), "src");
  GstElement *sink_el = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (src_el) {
    g_object_set(src_el, "location", src.c_str(), nullptr);
    gst_object_unref(src_el);
  }
  if (sink_el) {
    g_object_set(sink_el, "location", destination.c_str(), nullptr);
    gst_object_unref(sink_el);
  }
  TranscoderGstProperties::ApplyStoredProperties(pipeline, format);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  GstBus *bus = gst_element_get_bus(pipeline);
  bool ok = false;
  int idle = 0;
  while (true) {
    if (cancelled_) {
      log_.push_back("Cancelled");
      break;
    }
    GstMessage *message = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
                                                     static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    while (g_main_context_pending(nullptr)) {
      g_main_context_iteration(nullptr, FALSE);
    }
    if (!message) {
      if (++idle >= 300) {
        log_.push_back("Timed out transcoding " + src);
        break;
      }
      continue;
    }
    idle = 0;
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
      ok = true;
      gst_message_unref(message);
      break;
    }
    GError *gst_error = nullptr;
    gst_message_parse_error(message, &gst_error, nullptr);
    log_.push_back(gst_error ? gst_error->message : "Transcode failed");
    if (gst_error) {
      g_error_free(gst_error);
    }
    gst_message_unref(message);
    break;
  }
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  if (ok) {
    log_.push_back("Wrote " + destination);
  } else {
    FileUtils::Remove(destination);
  }
  LogLine.Emit(log_.back());
  return ok;
}

void Transcoder::Start() {
  cancelled_ = false;
  finished_success_ = 0;
  finished_failed_ = 0;
  const int total = static_cast<int>(jobs_.size());
  int i = 0;
  while (i < static_cast<int>(jobs_.size())) {
    if (cancelled_) {
      break;
    }
    const Job job = jobs_[static_cast<size_t>(i)];
    if (TranscodeFile(job.song, job.destination, job.format)) {
      ++finished_success_;
    } else {
      ++finished_failed_;
    }
    ++i;
    Progress.Emit(finished_success_ + finished_failed_, total);
    while (g_main_context_pending(nullptr)) {
      g_main_context_iteration(nullptr, FALSE);
    }
  }
  jobs_.clear();
  Finished.Emit();
}

void Transcoder::Cancel() {
  cancelled_ = true;
  jobs_.clear();
}

std::string Transcoder::FormatName(Format format) { return PresetFor(format).name; }

std::string Transcoder::Extension(Format format) { return PresetFor(format).extension; }
