#ifndef STRAWBERRY_EDITTAGSUMMARYFIELDS_H
#define STRAWBERRY_EDITTAGSUMMARYFIELDS_H

#include "core/song.h"
#include "dialogs/edittagsummarylabels.h"
#include "utilities/fileutils.h"
#include "utilities/timeutils.h"

#include <glib.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace EditTagSummaryFields {

struct Row {
  const char *label = nullptr;
  std::string value;
};

inline std::string FormatUnixTime(int64_t ts) {
  if (ts <= 0) {
    return {};
  }
  GDateTime *dt = g_date_time_new_from_unix_local(ts);
  if (!dt) {
    return {};
  }
  gchar *text = g_date_time_format(dt, "%Y-%m-%d %H:%M");
  std::string out = text ? text : "";
  g_free(text);
  g_date_time_unref(dt);
  return out;
}

inline std::string YesNo(bool value) { return value ? EditTagSummaryLabels::Yes() : EditTagSummaryLabels::No(); }

inline std::string ArtPath(const std::string &uri) {
  if (uri.empty()) {
    return EditTagSummaryLabels::None();
  }
  const std::string path = FileUtils::PathFromUri(uri);
  return path.empty() ? uri : path;
}

inline std::string Loudness(const std::optional<double> &value, const char *unit) {
  if (!value) {
    return {};
  }
  return std::to_string(*value) + (unit ? std::string(" ") + unit : std::string());
}

inline std::vector<Row> Rows(const Song &song) {
  const std::string path = FileUtils::PathFromUri(song.url());
  std::vector<Row> rows = {
      {EditTagSummaryLabels::Filename(), path.empty() ? song.url() : FileUtils::BaseName(path)},
      {EditTagSummaryLabels::Path(), path.empty() ? std::string() : FileUtils::DirName(path)},
      {EditTagSummaryLabels::FileType(), Song::FiletypeToString(song.filetype())},
      {EditTagSummaryLabels::Length(), Utilities::PrettyTimeNanosec(song.length_nanosec())},
      {EditTagSummaryLabels::BitRate(), song.bitrate() > 0 ? std::to_string(song.bitrate()) + " kbps" : std::string()},
      {EditTagSummaryLabels::SampleRate(), song.samplerate() > 0 ? std::to_string(song.samplerate()) + " Hz" : std::string()},
      {EditTagSummaryLabels::BitDepth(), song.bitdepth() > 0 ? std::to_string(song.bitdepth()) + " Bit" : std::string()},
      {EditTagSummaryLabels::DateCreated(), FormatUnixTime(song.ctime())},
      {EditTagSummaryLabels::DateModified(), FormatUnixTime(song.mtime())},
      {EditTagSummaryLabels::ArtEmbedded(), YesNo(song.art_embedded())},
      {EditTagSummaryLabels::ArtAutomatic(), ArtPath(song.art_automatic())},
      {EditTagSummaryLabels::ArtManual(), ArtPath(song.art_manual())},
      {EditTagSummaryLabels::ArtUnset(), YesNo(song.art_unset())},
      {EditTagSummaryLabels::EbuIntegrated(), Loudness(song.ebur128_integrated_loudness_lufs(), "LUFS")},
      {EditTagSummaryLabels::EbuRange(), Loudness(song.ebur128_loudness_range_lu(), "LU")},
      {EditTagSummaryLabels::PlayCount(), std::to_string(song.playcount())},
      {EditTagSummaryLabels::SkipCount(), std::to_string(song.skipcount())},
  };
  if (song.filesize() < 0) {
    rows.insert(rows.begin() + 3, { "File size", EditTagSummaryLabels::Unknown() });
  } else if (song.filesize() > 0) {
    rows.insert(rows.begin() + 3, { "File size", FileUtils::PrettySize(song.filesize()) });
  }
  return rows;
}

}  // namespace EditTagSummaryFields

#endif
