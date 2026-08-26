#ifndef STRAWBERRY_TRANSCODERSETTINGSPAGE_H
#define STRAWBERRY_TRANSCODERSETTINGSPAGE_H

#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsfields.h"
#include "transcoder/transcodeui.h"

#include <adwaita.h>

#include <string>
#include <utility>
#include <vector>

class Application;
class Settings;

namespace TranscoderSettingsPage {

AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);

inline const char *IntroText() {
  return "These settings are used in the \"Transcode Music\" dialog, and when converting music before copying it to a device.";
}

inline std::vector<Transcoder::Format> TabFormats() {
  return {
      Transcoder::Format::FLAC,      Transcoder::Format::WavPack, Transcoder::Format::OggVorbis, Transcoder::Format::Opus,
      Transcoder::Format::Speex,     Transcoder::Format::AAC,     Transcoder::Format::ASF,       Transcoder::Format::MP3,
  };
}

inline const char *TabLabel(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::FLAC:
      return "FLAC";
    case Transcoder::Format::WavPack:
      return "WavPack";
    case Transcoder::Format::OggVorbis:
      return "Vorbis";
    case Transcoder::Format::Opus:
      return "Opus";
    case Transcoder::Format::Speex:
      return "Speex";
    case Transcoder::Format::AAC:
      return "AAC";
    case Transcoder::Format::ASF:
      return "ASF (WMA)";
    case Transcoder::Format::MP3:
      return "MP3";
  }
  return "FLAC";
}

inline const char *TabId(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::FLAC:
      return "flac";
    case Transcoder::Format::WavPack:
      return "wavpack";
    case Transcoder::Format::OggVorbis:
      return "vorbis";
    case Transcoder::Format::Opus:
      return "opus";
    case Transcoder::Format::Speex:
      return "speex";
    case Transcoder::Format::AAC:
      return "aac";
    case Transcoder::Format::ASF:
      return "asf";
    case Transcoder::Format::MP3:
      return "mp3";
  }
  return "flac";
}

inline const char *GroupName(Transcoder::Format format) {
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
  }
  return "Transcoder";
}

enum class FieldKind { Quality, Bitrate, Mp3 };

inline FieldKind FieldsFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::MP3:
      return FieldKind::Mp3;
    case Transcoder::Format::AAC:
    case Transcoder::Format::Opus:
    case Transcoder::Format::ASF:
      return FieldKind::Bitrate;
    default:
      return FieldKind::Quality;
  }
}

inline int QualityMax(Transcoder::Format format) { return format == Transcoder::Format::FLAC ? 8 : 10; }

inline const char *QualityTitle(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::FLAC:
      return "Compression (0–8)";
    case Transcoder::Format::WavPack:
      return "Mode (0 fast … 10 extra)";
    default:
      return "Quality (0–10)";
  }
}

inline std::vector<std::pair<std::string, std::string>> DefaultFormatChoices() {
  return {
      {"audio/mpeg", "MP3"},         {"audio/mp4", "AAC"},       {"audio/x-flac", "FLAC"},     {"audio/x-vorbis", "Ogg Vorbis"},
      {"audio/x-opus", "Opus"},      {"audio/x-speex", "Speex"}, {"audio/x-wavpack", "WavPack"}, {"audio/x-wma", "ASF"},
  };
}

inline bool GroupMatches(Transcoder::Format format) { return GroupName(format) == TranscoderOptionsFields::GroupFor(format); }

inline const char *FormatKeyFor(Transcoder::Format format) {
  switch (format) {
    case Transcoder::Format::MP3:
      return TranscodeUi::FormatKey(0);
    case Transcoder::Format::AAC:
      return TranscodeUi::FormatKey(1);
    case Transcoder::Format::FLAC:
      return TranscodeUi::FormatKey(2);
    case Transcoder::Format::OggVorbis:
      return TranscodeUi::FormatKey(3);
    case Transcoder::Format::Opus:
      return TranscodeUi::FormatKey(4);
    case Transcoder::Format::Speex:
      return TranscodeUi::FormatKey(5);
    case Transcoder::Format::WavPack:
      return TranscodeUi::FormatKey(6);
    case Transcoder::Format::ASF:
      return TranscodeUi::FormatKey(7);
  }
  return TranscodeUi::FormatKey(3);
}

}  // namespace TranscoderSettingsPage

#endif
