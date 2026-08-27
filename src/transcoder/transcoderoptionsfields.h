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
      return "Transcoder/faac";
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
      return "Transcoder/ffenc_wmav2";
    case Transcoder::Format::WAV:
      return "Transcoder/wavenc";
    case Transcoder::Format::OggFlac:
      return "Transcoder/flacenc";
    case Transcoder::Format::ALAC:
      return "Transcoder/avenc_alac";
  }
  return TranscoderSettings::kSettingsGroup;
}

inline const char *LegacyGroupFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::AAC:
      return "Transcoder/avenc_aac";
    case Transcoder::Format::ASF:
      return "Transcoder/avenc_wmav2";
    default:
      return nullptr;
  }
}

inline void BeginStoredGroup(Settings *settings, Transcoder::Format format) {
  if (!settings) {
    return;
  }
  settings->BeginGroup(GroupFor(format));
  const char *legacy = LegacyGroupFor(format);
  if (!legacy) {
    return;
  }
  if (settings->Contains("bitrate") || settings->Contains("quality") || settings->Contains("profile") || settings->Contains("managed")) {
    return;
  }
  settings->BeginGroup(legacy);
}

inline int NormalizeBitrateBps(int stored, int fallback) {
  if (stored <= 0) {
    return fallback;
  }
  if (stored < 1000) {
    return stored * 1000;
  }
  return stored;
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

struct Vorbis {
  int quality = 5;
  bool managed = false;
  int bitrate_bps = -1;
  int min_bitrate_bps = -1;
  int max_bitrate_bps = -1;

  void ApplyQuality(int value) { quality = std::clamp(value, 0, 10); }

  void Load() {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::OggVorbis));
    const double stored = settings.DoubleValue("quality", static_cast<double>(quality) / 10.0);
    if (stored > 0 && stored <= 1.0) {
      quality = std::clamp(static_cast<int>(stored * 10.0 + 0.5), 0, 10);
    } else {
      quality = settings.IntValue("quality", quality);
    }
    managed = settings.BoolValue("managed", managed);
    bitrate_bps = settings.IntValue("bitrate", bitrate_bps);
    min_bitrate_bps = settings.IntValue("min-bitrate", min_bitrate_bps);
    max_bitrate_bps = settings.IntValue("max-bitrate", max_bitrate_bps);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::OggVorbis));
    settings.SetDoubleValue("quality", static_cast<double>(quality) / 10.0);
    settings.SetBoolValue("managed", managed);
    settings.SetIntValue("bitrate", bitrate_bps);
    settings.SetIntValue("min-bitrate", min_bitrate_bps);
    settings.SetIntValue("max-bitrate", max_bitrate_bps);
    settings.Sync();
  }

  std::string Pipeline() const {
    if (managed) {
      std::string fragment = "vorbisenc managed=true";
      if (bitrate_bps > 0) {
        fragment += " bitrate=" + std::to_string(bitrate_bps);
      }
      if (min_bitrate_bps > 0) {
        fragment += " min-bitrate=" + std::to_string(min_bitrate_bps);
      }
      if (max_bitrate_bps > 0) {
        fragment += " max-bitrate=" + std::to_string(max_bitrate_bps);
      }
      return fragment + " ! oggmux";
    }
    return "vorbisenc quality=" + std::to_string(static_cast<double>(quality) / 10.0) + " ! oggmux";
  }
};

struct Speex {
  int quality = 5;
  int bitrate_bps = 0;
  int mode = 0;
  bool vbr = false;
  int abr_bps = 0;
  bool vad = false;
  bool dtx = false;
  int complexity = 3;
  int nframes = 1;

  void ApplyQuality(int value) { quality = std::clamp(value, 0, 10); }

  void Load() {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::Speex));
    quality = settings.IntValue("quality", quality);
    bitrate_bps = settings.IntValue("bitrate", bitrate_bps);
    mode = settings.IntValue("mode", mode);
    vbr = settings.BoolValue("vbr", vbr);
    abr_bps = settings.IntValue("abr", abr_bps);
    vad = settings.BoolValue("vad", vad);
    dtx = settings.BoolValue("dtx", dtx);
    complexity = settings.IntValue("complexity", complexity);
    nframes = settings.IntValue("nframes", nframes);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::Speex));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", bitrate_bps);
    settings.SetIntValue("mode", mode);
    settings.SetBoolValue("vbr", vbr);
    settings.SetIntValue("abr", abr_bps);
    settings.SetBoolValue("vad", vad);
    settings.SetBoolValue("dtx", dtx);
    settings.SetIntValue("complexity", complexity);
    settings.SetIntValue("nframes", nframes);
    settings.Sync();
  }

  std::string Pipeline() const {
    std::string fragment = "speexenc quality=" + std::to_string(std::clamp(quality, 0, 10));
    if (bitrate_bps > 0) {
      fragment += " bitrate=" + std::to_string(bitrate_bps);
    }
    if (mode > 0) {
      fragment += " mode=" + std::to_string(mode);
    }
    if (vbr) {
      fragment += " vbr=true";
    }
    if (abr_bps > 0) {
      fragment += " abr=" + std::to_string(abr_bps);
    }
    if (vad) {
      fragment += " vad=true";
    }
    if (dtx) {
      fragment += " dtx=true";
    }
    fragment += " complexity=" + std::to_string(std::clamp(complexity, 0, 10));
    fragment += " nframes=" + std::to_string(std::max(1, nframes));
    return fragment + " ! oggmux";
  }
};

struct Aac {
  int quality = 5;
  int bitrate_bps = 128000;
  int profile = 2;
  bool tns = false;
  bool midside = true;
  int shortctl = 0;

  void ApplyQuality(int value) {
    quality = std::clamp(value, 0, 10);
    bitrate_bps = TranscoderOptionsInterface::BitrateKbps(quality, 64, 320) * 1000;
  }

  void Load() {
    Settings settings;
    BeginStoredGroup(&settings, Transcoder::Format::AAC);
    quality = settings.IntValue("quality", quality);
    bitrate_bps = NormalizeBitrateBps(settings.IntValue("bitrate", bitrate_bps), bitrate_bps);
    profile = settings.IntValue("profile", profile);
    tns = settings.BoolValue("tns", tns);
    midside = settings.BoolValue("midside", midside);
    shortctl = settings.IntValue("shortctl", shortctl);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::AAC));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", bitrate_bps);
    settings.SetIntValue("profile", profile);
    settings.SetBoolValue("tns", tns);
    settings.SetBoolValue("midside", midside);
    settings.SetIntValue("shortctl", shortctl);
    settings.Sync();
  }

  std::string Pipeline() const {
    std::string fragment = "avenc_aac bitrate=" + std::to_string(std::max(8000, bitrate_bps));
    fragment += " profile=" + std::to_string(profile);
    if (tns) {
      fragment += " tns=true";
    }
    if (midside) {
      fragment += " midside=true";
    }
    fragment += " shortctl=" + std::to_string(shortctl);
    return fragment + " ! mp4mux";
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
    BeginStoredGroup(&settings, format);
    quality = settings.IntValue("quality", quality);
    if (settings.Contains("bitrate")) {
      const int bitrate = settings.IntValue("bitrate", Bitrate());
      const int kbps = bitrate >= 1000 ? bitrate / 1000 : bitrate;
      if (max_kbps > min_kbps) {
        quality = std::clamp((kbps - min_kbps) * 10 / (max_kbps - min_kbps), 0, 10);
      }
    }
  }

  void Save(Transcoder::Format format) const {
    Settings settings;
    settings.BeginGroup(GroupFor(format));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", Bitrate() * 1000);
    settings.Sync();
  }
};

struct Opus {
  int quality = 5;
  int bitrate_bps = 320000;

  void ApplyQuality(int value) {
    quality = std::clamp(value, 0, 10);
    bitrate_bps = TranscoderOptionsInterface::BitrateKbps(quality, 48, 256) * 1000;
  }

  void Load() {
    Settings settings;
    BeginStoredGroup(&settings, Transcoder::Format::Opus);
    quality = settings.IntValue("quality", quality);
    bitrate_bps = NormalizeBitrateBps(settings.IntValue("bitrate", bitrate_bps), bitrate_bps);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::Opus));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", bitrate_bps);
    settings.Sync();
  }

  std::string Pipeline() const { return "opusenc bitrate=" + std::to_string(std::max(6000, bitrate_bps)) + " ! oggmux"; }
};

struct Asf {
  int quality = 5;
  int bitrate_bps = 320000;

  void ApplyQuality(int value) {
    quality = std::clamp(value, 0, 10);
    bitrate_bps = TranscoderOptionsInterface::BitrateKbps(quality, 64, 192) * 1000;
  }

  void Load() {
    Settings settings;
    BeginStoredGroup(&settings, Transcoder::Format::ASF);
    quality = settings.IntValue("quality", quality);
    bitrate_bps = NormalizeBitrateBps(settings.IntValue("bitrate", bitrate_bps), bitrate_bps);
  }

  void Save() const {
    Settings settings;
    settings.BeginGroup(GroupFor(Transcoder::Format::ASF));
    settings.SetIntValue("quality", quality);
    settings.SetIntValue("bitrate", bitrate_bps);
    settings.Sync();
  }

  std::string Pipeline() const { return "avenc_wmav2 bitrate=" + std::to_string(std::max(8000, bitrate_bps)) + " ! asfmux"; }
};

}  // namespace TranscoderOptionsFields

#endif
