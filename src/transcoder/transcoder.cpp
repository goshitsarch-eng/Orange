#include "transcoder/transcoder.h"

#include "core/logging.h"
#include "transcoder/transcodergstproperties.h"
#include "transcoder/transcoderoptionsdialog.h"
#include "transcoder/transcoderoptionsinterface.h"
#include "transcoder/transcoderprogress.h"
#include "utilities/fileutils.h"

#include <glib.h>
#include <gst/gst.h>

#include <algorithm>

Transcoder::Transcoder() {
  max_threads_ = TranscoderProgress::ClampMaxThreads(static_cast<int>(g_get_num_processors()));
}

Transcoder::~Transcoder() {
  *alive_ = false;
  Cancel();
}

void Transcoder::AddJob(const Song &song, const std::string &destination, Format format) {
  jobs_.push_back({song, destination, format});
}

void Transcoder::set_max_threads(int count) { max_threads_ = TranscoderProgress::ClampMaxThreads(count); }

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
    case Format::WAV:
      return {format, "Wav", "wav", {}, "audio/x-wav"};
    case Format::OggFlac:
      return {format, "Ogg FLAC", "ogg", "audio/x-flac", "application/ogg"};
    case Format::ALAC:
      return {format, "ALAC", "m4a", "audio/x-alac", "audio/mp4"};
  }
  return {Format::FLAC, "FLAC", "flac", "audio/x-flac", {}};
}

Transcoder::Format Transcoder::FormatFromFileType(Song::FileType type) {
  switch (type) {
    case Song::FileType::WAV:
      return Format::WAV;
    case Song::FileType::FLAC:
      return Format::FLAC;
    case Song::FileType::WavPack:
      return Format::WavPack;
    case Song::FileType::OggFlac:
      return Format::OggFlac;
    case Song::FileType::OggVorbis:
      return Format::OggVorbis;
    case Song::FileType::OggOpus:
      return Format::Opus;
    case Song::FileType::OggSpeex:
      return Format::Speex;
    case Song::FileType::MPEG:
      return Format::MP3;
    case Song::FileType::MP4:
      return Format::AAC;
    case Song::FileType::ASF:
      return Format::ASF;
    case Song::FileType::ALAC:
      return Format::ALAC;
    default:
      return Format::FLAC;
  }
}

Transcoder::Preset Transcoder::PresetForFileType(Song::FileType type) { return PresetFor(FormatFromFileType(type)); }

std::vector<Transcoder::Preset> Transcoder::GetAllPresets() {
  // Qt Transcoder::GetAllPresets
  return {
      PresetForFileType(Song::FileType::WAV),      PresetForFileType(Song::FileType::FLAC),      PresetForFileType(Song::FileType::WavPack),
      PresetForFileType(Song::FileType::OggFlac),  PresetForFileType(Song::FileType::OggVorbis), PresetForFileType(Song::FileType::OggOpus),
      PresetForFileType(Song::FileType::OggSpeex), PresetForFileType(Song::FileType::MPEG),     PresetForFileType(Song::FileType::MP4),
      PresetForFileType(Song::FileType::ASF),      PresetForFileType(Song::FileType::ALAC),
  };
}

std::string Transcoder::PipelineFor(Format format, int quality) {
  auto options = TranscoderOptionsDialog::OptionsFor(format);
  options->Load();
  if (quality >= 0) {
    options->ApplyQuality(quality);
  }
  return options->PipelineFragment();
}

std::string Transcoder::JobInput(const Job &job) { return FileUtils::PathFromUri(job.song.url()); }

GstElement *Transcoder::CreatePipeline(const Job &job, std::string *error) {
  const std::string src = JobInput(job);
  if (!FileUtils::IsFile(src)) {
    if (error) {
      *error = "Missing source: " + src;
    }
    return nullptr;
  }
  FileUtils::WriteFile(job.destination, {});
  const std::string fragment = PipelineFor(job.format, quality_);
  const std::string desc = "filesrc name=src ! decodebin ! audioconvert ! audioresample ! " + fragment + " ! filesink name=sink";
  GError *gst_error = nullptr;
  GstElement *pipeline = gst_parse_launch(desc.c_str(), &gst_error);
  if (!pipeline) {
    const std::string message = gst_error ? gst_error->message : "Could not create transcoder pipeline";
    if (gst_error) {
      g_error_free(gst_error);
    }
    if (error) {
      *error = message + " (" + fragment + ")";
    }
    FileUtils::Remove(job.destination);
    return nullptr;
  }
  GstElement *src_el = gst_bin_get_by_name(GST_BIN(pipeline), "src");
  GstElement *sink_el = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
  if (src_el) {
    g_object_set(src_el, "location", src.c_str(), nullptr);
    gst_object_unref(src_el);
  }
  if (sink_el) {
    g_object_set(sink_el, "location", job.destination.c_str(), nullptr);
    gst_object_unref(sink_el);
  }
  TranscoderGstProperties::ApplyStoredProperties(pipeline, job.format);
  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  return pipeline;
}

bool Transcoder::TranscodeFile(const Song &song, const std::string &destination, Format format) {
  const Job job{song, destination, format};
  std::string error;
  GstElement *pipeline = CreatePipeline(job, &error);
  if (!pipeline) {
    log_.push_back(error.empty() ? "Transcode failed" : error);
    LogLine.Emit(log_.back());
    return false;
  }
  GstBus *bus = gst_element_get_bus(pipeline);
  bool ok = false;
  int idle = 0;
  while (true) {
    if (cancelled_) {
      log_.push_back("Cancelled");
      break;
    }
    GstMessage *message = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    while (g_main_context_pending(nullptr)) {
      g_main_context_iteration(nullptr, FALSE);
    }
    if (!message) {
      if (++idle >= 300) {
        log_.push_back("Timed out transcoding " + JobInput(job));
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
  total_ = static_cast<int>(jobs_.size());
  log_.push_back("Transcoding " + std::to_string(total_) + " files using " + std::to_string(max_threads_) + " threads");
  LogLine.Emit(log_.back());
  PumpJobs();
}

void Transcoder::PumpJobs() {
  while (TranscoderProgress::ShouldStartNextJob(static_cast<int>(current_jobs_.size()), static_cast<int>(jobs_.size()), max_threads_)) {
    const Job job = jobs_.front();
    jobs_.erase(jobs_.begin());
    if (StartJob(job)) {
      continue;
    }
    ++finished_failed_;
    JobComplete.Emit(JobInput(job), job.destination, false);
    Progress.Emit(finished_success_ + finished_failed_, total_);
  }
  if (TranscoderProgress::AllIdle(static_cast<int>(current_jobs_.size()), static_cast<int>(jobs_.size()))) {
    Finished.Emit();
    AllJobsComplete.Emit();
  }
}

bool Transcoder::StartJob(const Job &job) {
  const std::string input = JobInput(job);
  log_.push_back("Starting " + input);
  LogLine.Emit(log_.back());
  std::string error;
  GstElement *pipeline = CreatePipeline(job, &error);
  if (!pipeline) {
    if (!error.empty()) {
      log_.push_back(error);
      LogLine.Emit(log_.back());
    }
    return false;
  }
  GstBus *bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, Transcoder::BusWatch, this);
  gst_object_unref(bus);
  current_jobs_.push_back({job, pipeline});
  return true;
}

gboolean Transcoder::BusWatch(GstBus *, GstMessage *msg, gpointer data) {
  auto *self = static_cast<Transcoder *>(data);
  const GstMessageType type = GST_MESSAGE_TYPE(msg);
  if (type != GST_MESSAGE_EOS && type != GST_MESSAGE_ERROR) {
    return TRUE;
  }
  GstObject *obj = GST_MESSAGE_SRC(msg);
  while (obj && !GST_IS_PIPELINE(obj)) {
    obj = GST_OBJECT_PARENT(obj);
  }
  if (!obj) {
    return TRUE;
  }
  auto *finish = new FinishData();
  finish->alive = self->alive_;
  finish->self = self;
  finish->pipeline = GST_ELEMENT(obj);
  finish->success = type == GST_MESSAGE_EOS;
  finish->generation = self->finish_generation_;
  if (type == GST_MESSAGE_ERROR) {
    GError *gst_error = nullptr;
    gst_message_parse_error(msg, &gst_error, nullptr);
    finish->error = gst_error ? gst_error->message : "Transcode failed";
    if (gst_error) {
      g_error_free(gst_error);
    }
  }
  g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, Transcoder::FinishIdle, finish, +[](gpointer p) { delete static_cast<FinishData *>(p); });
  return FALSE;
}

gboolean Transcoder::FinishIdle(gpointer data) {
  auto *finish = static_cast<FinishData *>(data);
  if (*finish->alive && finish->generation == finish->self->finish_generation_) {
    finish->self->FinishJob(finish->pipeline, finish->success, finish->error);
  }
  return G_SOURCE_REMOVE;
}

void Transcoder::FinishJob(GstElement *pipeline, bool success, const std::string &error) {
  auto it = std::find_if(current_jobs_.begin(), current_jobs_.end(), [pipeline](const CurrentJob &job) { return job.pipeline == pipeline; });
  if (it == current_jobs_.end()) {
    return;
  }
  const Job job = it->job;
  GstBus *bus = gst_element_get_bus(pipeline);
  if (bus) {
    gst_bus_remove_watch(bus);
    gst_object_unref(bus);
  }
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  current_jobs_.erase(it);
  if (success) {
    ++finished_success_;
    log_.push_back("Wrote " + job.destination);
  } else {
    ++finished_failed_;
    FileUtils::Remove(job.destination);
    log_.push_back(error.empty() ? "Transcode failed" : error);
  }
  LogLine.Emit(log_.back());
  JobComplete.Emit(JobInput(job), job.destination, success);
  Progress.Emit(finished_success_ + finished_failed_, total_);
  PumpJobs();
}

void Transcoder::StopCurrentJobs() {
  ++finish_generation_;
  std::vector<CurrentJob> running;
  running.swap(current_jobs_);
  for (CurrentJob &job : running) {
    if (!job.pipeline) {
      continue;
    }
    GstBus *bus = gst_element_get_bus(job.pipeline);
    if (bus) {
      gst_bus_remove_watch(bus);
      gst_object_unref(bus);
    }
    if (gst_element_set_state(job.pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_ASYNC) {
      gst_element_get_state(job.pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
    }
    gst_object_unref(job.pipeline);
  }
}

void Transcoder::Cancel() {
  cancelled_ = true;
  jobs_.clear();
  StopCurrentJobs();
}

std::map<std::string, float> Transcoder::GetProgress() const {
  std::map<std::string, float> progress;
  for (const CurrentJob &job : current_jobs_) {
    if (!job.pipeline) {
      continue;
    }
    gint64 position = 0;
    gint64 duration = 0;
    gst_element_query_position(job.pipeline, GST_FORMAT_TIME, &position);
    gst_element_query_duration(job.pipeline, GST_FORMAT_TIME, &duration);
    progress[JobInput(job.job)] = TranscoderProgress::FractionFromPosition(position, duration);
  }
  return progress;
}

std::string Transcoder::FormatName(Format format) { return PresetFor(format).name; }

std::string Transcoder::Extension(Format format) { return PresetFor(format).extension; }
