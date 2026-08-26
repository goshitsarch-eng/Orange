#include "tagreader/tagreadergme.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <taglib/apefile.h>
#include <taglib/tag.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

std::string Latin1Field(const std::string &data, size_t offset, size_t length) {
  if (offset >= data.size()) {
    return {};
  }
  const size_t n = std::min(length, data.size() - offset);
  std::string field(data.data() + offset, n);
  const auto nul = field.find('\0');
  if (nul != std::string::npos) {
    field.resize(nul);
  }
  return StrUtils::Trim(field);
}

std::vector<std::string> SplitUtf16Le(const std::string &data) {
  std::vector<std::string> strings;
  std::string current;
  for (size_t i = 0; i + 1 < data.size(); i += 2) {
    const gunichar ch = static_cast<unsigned char>(data[i]) | (static_cast<gunichar>(static_cast<unsigned char>(data[i + 1])) << 8);
    if (ch == 0) {
      strings.push_back(current);
      current.clear();
      continue;
    }
    gchar utf8[8] = {};
    const gint len = g_unichar_to_utf8(ch, utf8);
    if (len > 0) {
      current.append(utf8, static_cast<size_t>(len));
    }
  }
  if (!current.empty()) {
    strings.push_back(current);
  }
  return strings;
}

}  // namespace

bool TagReaderGME::IsSupported(const std::string &filename) {
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(filename));
  return ext == "spc" || ext == "vgm";
}

uint32_t TagReaderGME::UnpackBytes32(const char *bytes, size_t length) {
  uint32_t value = 0;
  for (size_t i = 0; i < length && i < 4; ++i) {
    value |= static_cast<uint32_t>(static_cast<unsigned char>(bytes[i])) << (8 * i);
  }
  return value;
}

uint64_t TagReaderGME::ConvertSPCStringToNum(const char *bytes, size_t length) {
  uint64_t result = 0;
  for (size_t i = 0; i < length; ++i) {
    const unsigned num = static_cast<unsigned>(static_cast<unsigned char>(bytes[i]) - '0');
    if (num > 9) {
      break;
    }
    result = (result * 10) + num;
  }
  return result;
}

bool TagReaderGME::GetPlaybackLengthMs(const char *sample_count, const char *loop_count, uint64_t *out_length_ms) {
  if (!sample_count || !loop_count || !out_length_ms) {
    return false;
  }
  const uint64_t samples = UnpackBytes32(sample_count, 4);
  if (samples == 0) {
    return false;
  }
  const uint64_t loop_samples = UnpackBytes32(loop_count, 4);
  if (loop_samples == 0) {
    *out_length_ms = samples * 1000 / kSampleTimebase;
    return true;
  }
  const uint64_t intro_ms = (samples - loop_samples) * 1000 / kSampleTimebase;
  const uint64_t loop_ms = loop_samples * 1000 / kSampleTimebase;
  *out_length_ms = intro_ms + (loop_ms * 2) + kGstGmeLoopTimeMs;
  return true;
}

bool TagReaderGME::ReadSpcData(const std::string &data, Song *song) {
  if (!song || data.size() < 33 || data.compare(0, 11, "SNES-SPC700") != 0) {
    return false;
  }
  song->set_title(Latin1Field(data, kSongTitleOffset, kFieldSize));
  song->set_album(Latin1Field(data, kGameTitleOffset, kFieldSize));
  song->set_artist(Latin1Field(data, kArtistOffset, kFieldSize));
  if (data.size() >= static_cast<size_t>(kIntroLengthOffset + kIntroLengthSize)) {
    const char *length_bytes = data.data() + kIntroLengthOffset;
    uint64_t length_in_sec = ConvertSPCStringToNum(length_bytes, kIntroLengthSize);
    if (length_in_sec == 0 || length_in_sec >= 0x1FFF) {
      length_in_sec = static_cast<uint64_t>(static_cast<unsigned char>(length_bytes[0])) |
                      (static_cast<uint64_t>(static_cast<unsigned char>(length_bytes[1])) << 8) |
                      (static_cast<uint64_t>(static_cast<unsigned char>(length_bytes[2])) << 16);
    }
    if (length_in_sec < 0x1FFF) {
      song->set_length_nanosec(static_cast<int64_t>(length_in_sec * 1000000000LL));
    }
  }
  song->set_valid(true);
  song->set_filetype(Song::FileType::SPC);
  return true;
}

bool TagReaderGME::ReadVgmData(const std::string &data, Song *song) {
  if (!song || data.size() < 0x24 || data.compare(0, 4, "Vgm ") != 0) {
    return false;
  }
  const uint64_t gd3_ptr = UnpackBytes32(data.data() + kGd3TagPtr, 4);
  uint64_t length_ms = 0;
  if (!GetPlaybackLengthMs(data.data() + kSampleCount, data.data() + kLoopSampleCount, &length_ms)) {
    return false;
  }
  const size_t gd3_offset = static_cast<size_t>(kGd3TagPtr) + static_cast<size_t>(gd3_ptr);
  if (gd3_offset + 12 > data.size()) {
    return false;
  }
  const uint32_t gd3_length = UnpackBytes32(data.data() + gd3_offset + 8, 4);
  const size_t payload_off = gd3_offset + 12;
  if (payload_off > data.size()) {
    return false;
  }
  const size_t payload_len = std::min(static_cast<size_t>(gd3_length), data.size() - payload_off);
  const std::vector<std::string> strings = SplitUtf16Le(data.substr(payload_off, payload_len));
  if (strings.size() < 10) {
    return false;
  }
  song->set_title(strings[0]);
  song->set_album(strings[2]);
  song->set_artist(strings[6]);
  if (strings[8].size() >= 4) {
    song->set_year(std::atoi(strings[8].substr(0, 4).c_str()));
  }
  song->set_length_nanosec(static_cast<int64_t>(length_ms * 1000000LL));
  song->set_valid(true);
  song->set_filetype(Song::FileType::VGM);
  return true;
}

bool TagReaderGME::ReadFile(const std::string &filename, Song *song) {
  if (!song || !IsSupported(filename)) {
    return false;
  }
  const std::string data = FileUtils::ReadFile(filename);
  const std::string ext = StrUtils::ToLower(FileUtils::Extension(filename));
  bool ok = false;
  if (ext == "spc") {
    ok = ReadSpcData(data, song);
    if (ok) {
      TagLib::APE::File ape(filename.c_str());
      if (ape.hasAPETag() && ape.tag()) {
        const TagLib::Tag *tag = ape.tag();
        auto from = [](const TagLib::String &value) { return value.to8Bit(true); };
        if (!tag->artist().isEmpty()) {
          song->set_artist(from(tag->artist()));
        }
        if (!tag->album().isEmpty()) {
          song->set_album(from(tag->album()));
        }
        if (!tag->title().isEmpty()) {
          song->set_title(from(tag->title()));
        }
        if (!tag->genre().isEmpty()) {
          song->set_genre(from(tag->genre()));
        }
        if (tag->track() > 0) {
          song->set_track(static_cast<int>(tag->track()));
        }
        if (tag->year() > 0) {
          song->set_year(static_cast<int>(tag->year()));
        }
      }
    }
  } else if (ext == "vgm") {
    ok = ReadVgmData(data, song);
  }
  return ok;
}
