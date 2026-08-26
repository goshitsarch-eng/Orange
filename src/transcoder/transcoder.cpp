#include "transcoder/transcoder.h"
#include "core/logging.h"
#include <gst/gst.h>
void Transcoder::AddJob(const Song &song, const std::string &destination, Format format) { jobs_.push_back({song, destination, format}); }
void Transcoder::Start() {
  int i = 0;
  for (const Job &job : jobs_) {
    LogInfo("Transcode %s -> %s (%s)", job.song.url().c_str(), job.destination.c_str(), FormatName(job.format).c_str());
    Progress.Emit(++i, static_cast<int>(jobs_.size()));
  }
  Finished.Emit();
}
void Transcoder::Cancel() { jobs_.clear(); }
std::string Transcoder::FormatName(Format format) {
  switch (format) {
    case Format::MP3: return "MP3"; case Format::AAC: return "AAC"; case Format::FLAC: return "FLAC";
    case Format::OggVorbis: return "Ogg Vorbis"; case Format::Opus: return "Opus"; case Format::Speex: return "Speex";
    case Format::WavPack: return "WavPack"; case Format::ASF: return "ASF";
  }
  return "Unknown";
}
std::string Transcoder::Extension(Format format) {
  switch (format) {
    case Format::MP3: return "mp3"; case Format::AAC: return "m4a"; case Format::FLAC: return "flac";
    case Format::OggVorbis: return "ogg"; case Format::Opus: return "opus"; case Format::Speex: return "spx";
    case Format::WavPack: return "wv"; case Format::ASF: return "wma";
  }
  return "bin";
}
