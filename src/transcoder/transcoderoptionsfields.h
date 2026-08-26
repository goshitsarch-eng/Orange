#ifndef STRAWBERRY_TRANSCODEROPTIONSFIELDS_H
#define STRAWBERRY_TRANSCODEROPTIONSFIELDS_H

#include "constants/transcodersettings.h"
#include "core/settings.h"
#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsinterface.h"

#include <algorithm>
#include <string>

namespace TranscoderOptionsFields {

inline std::string GroupFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::MP3:
      return "Transcoder/lamemp3enc";
    case Transcoder::Format::AAC:
      return "Transcoder/avenc_aac";
    case Transcoder::Format::FLAC:
      return "Transcoder/flacenc";
    case Transcoder::Format::OggVorbis:
      return "Transcoder/vorbisenc";
    case Transcoder::Format::Opus:
      return "Transcoder/opusenc";
    case Transcoder::Format::Speex:
      return "Transcoder/speexenc";
    case Transcoder::Format::WavPack:
      return "Transcoder/wavpackenc";
    case Transcoder::Format::ASF:
      return "Transcoder/avenc_wmav2";
  }
  return TranscoderSettings::kSettingsGroup;
}

struct Mp3 {
  int target = 1;
  int quality = 4;
  int bitrate = 176;
  bool cbr = false;
  bool mono = false;
  int engine_quality = 1;

  void ApplyQuality(int value) {
    quality = std::clamp(value, 0, 9);
    bitrate = 96 + std::clamp(value, 0, 10) * 16;
  }

  void Load() {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::MP3));
    target = settings.IntValue(TranscoderSettings::LameMP3Settings::kTarget, target);
    quality = settings.IntValue(TranscoderSettings::LameMP3Settings::kQuality, quality);
    bitrate = settings.IntValue(TranscoderSettings::LameMP3Settings::kBitrate, bitrate);
    cbr = settings.BoolValue(TranscoderSettings::LameMP3Settings::kCbr, cbr);
    mono = settings.BoolValue(TranscoderSettings::LameMP3Settings::kMono, mono);
    engine_quality = settings.IntValue(TranscoderSettings::LameMP3Settings::kEncodingEngineQuality, engine_quality);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::MP3));
    settings.SetIntValue(TranscoderSettings::LameMP3Settings::kTarget, target);
    settings.SetIntValue(TranscoderSettings::LameMP3Settings::kQuality, quality);
    settings.SetIntValue(TranscoderSettings::LameMP3Settings::kBitrate, bitrate);
    settings.SetBoolValue(TranscoderSettings::LameMP3Settings::kCbr, cbr);
    settings.SetBoolValue(TranscoderSettings::LameMP3Settings::kMono, mono);
    settings.SetIntValue(TranscoderSettings::LameMP3Settings::kEncodingEngineQuality, engine_quality);
    settings.Sync();
  }

  std::string Pipeline() const {
    std::string fragment = "lamemp3enc target=" + std::to_string(target);
    if (target == 0) {
      fragment += " quality=" + std::to_string(std::clamp(quality, 0, 9));
    } else {
      fragment += " bitrate=" + std::to_string(std::max(32, bitrate));
    }
    if (cbr) {
      fragment += " cbr=true";
    }
    if (mono) {
      fragment += " mono=true";
    }
    fragment += " encoding-engine-quality=" + std::to_string(std::clamp(engine_quality, 0, 2));
    fragment += " ! xingmux ! id3v2mux";
    return fragment;
  }
};

struct QualityEncoder {
  int quality = 5;
  int min_quality = 0;
  int max_quality = 10;

  void ApplyQuality(int value) { quality = std::clamp(value, min_quality, max_quality); }

  void Load(Transcoder::Format format, const char *key = "quality") {
    Settings settings;
    settings.BeginGroup(GroupFor(format));
    quality = settings.IntValue(key, quality);
  }

  void Save(Transcoder::Format format, const char *key = "quality") const {
    Settings settings;
    settings.BeginGroup(GroupFor(format));
    settings.SetIntValue(key, quality);
    settings.Sync();
  }
};

struct BitrateEncoder {
  int quality = 5;
  int min_kbps = 64;
  int max_kbps = 320;

  void ApplyQuality(int value) { quality = std::clamp(value, 0, 10); }

  int Bitrate() const { return TranscoderOptionsInterface::BitrateKbps(quality, min_kbps, max_kbps); }

  void Load(Transcoder::Format format) {
    Settings settings;
    settings.BeginGroup(GroupFor(format));
    quality = settings.IntValue("quality", quality);
    if (settings.Contains("bitrate")) {
      const int bitrate = settings.IntValue("bitrate", Bitrate());
      if (max_kbps > min_kbps) {
        quality = std::clamp((bitrate - min_kbps) * 10 / (max_kbps - min_kbps), 0, 10);
      }
    }
  }

  void Save(Transcoder::Format format) const {
    Settings settings;
    settings.BeginGroup(GroupFor(format));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", Bitrate());
    settings.Sync();
  }
};

}  // namespace TranscoderOptionsFields

#endif
