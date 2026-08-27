#include "context/contexttechnical.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"

#include <glib.h>

#include <cmath>

namespace ContextTechnical {

namespace {

std::string FormatTimestamp(int64_t unix_sec) {
  if (unix_sec <= 0) {
    return {};
  }
  GDateTime *dt = g_date_time_new_from_unix_local(unix_sec);
  if (!dt) {
    return {};
  }
  gchar *text = g_date_time_format(dt, "%Y-%m-%d %H:%M");
  std::string result = text ? text : "";
  g_free(text);
  g_date_time_unref(dt);
  return result;
}

}  // namespace

std::vector<std::pair<std::string, std::string>> Rows(const Song &song) {
  std::vector<std::pair<std::string, std::string>> rows;
  if (!song.is_valid() && song.url().empty()) {
    return rows;
  }
  const std::string filetype = Song::FiletypeToString(song.filetype());
  if (!filetype.empty() && filetype != "Unknown") {
    rows.emplace_back("Filetype", filetype);
  }
  if (song.length_nanosec() > 0) {
    rows.emplace_back("Length", Utilities::PrettyTimeNanosec(song.length_nanosec()));
  }
  if (song.samplerate() > 0) {
    rows.emplace_back("Samplerate", std::to_string(song.samplerate()) + " Hz");
  }
  if (song.bitdepth() > 0) {
    rows.emplace_back("Bit depth", std::to_string(song.bitdepth()) + " Bit");
  }
  if (song.bitrate() > 0) {
    rows.emplace_back("Bitrate", std::to_string(song.bitrate()) + " kbps");
  }
  if (song.year() > 0) {
    rows.emplace_back("Year", std::to_string(song.year()));
  }
  if (song.originalyear() > 0 && song.originalyear() != song.year()) {
    rows.emplace_back("Original year", std::to_string(song.originalyear()));
  }
  if (!song.genre().empty()) {
    rows.emplace_back("Genre", song.genre());
  }
  if (!song.composer().empty()) {
    rows.emplace_back("Composer", song.composer());
  }
  if (!song.performer().empty()) {
    rows.emplace_back("Performer", song.performer());
  }
  if (!song.grouping().empty()) {
    rows.emplace_back("Grouping", song.grouping());
  }
  if (!song.comment().empty()) {
    rows.emplace_back("Comment", song.comment());
  }
  if (song.disc() > 0) {
    rows.emplace_back("Disc", std::to_string(song.disc()));
  }
  if (song.track() > 0) {
    rows.emplace_back("Track", std::to_string(song.track()));
  }
  if (song.playcount() > 0) {
    rows.emplace_back("Play count", std::to_string(song.playcount()));
  }
  if (song.skipcount() > 0) {
    rows.emplace_back("Skip count", std::to_string(song.skipcount()));
  }
  if (song.rating() > 0.0f) {
    const int stars = static_cast<int>(std::lround(song.rating() * 5.0f));
    rows.emplace_back("Rating", std::to_string(stars) + " / 5");
  }
  if (!song.basefilename().empty()) {
    rows.emplace_back("Filename", song.basefilename());
  } else if (!song.url().empty()) {
    rows.emplace_back("URL", song.url());
  }
  if (song.filesize() > 0) {
    rows.emplace_back("Filesize", FileUtils::PrettySize(song.filesize()));
  }
  const std::string last_played = FormatTimestamp(song.lastplayed());
  if (!last_played.empty()) {
    rows.emplace_back("Last played", last_played);
  }
  return rows;
}

std::string Headline(const Song &song, const std::string &format) {
  if (!song.is_valid() && song.title().empty()) {
    return {};
  }
  return StrUtils::ReplaceMessage(format.empty() ? "%title% - %artist%" : format, song);
}

std::string Summary(const Song &song, const std::string &format) {
  if (!song.is_valid() && song.album().empty()) {
    return {};
  }
  return StrUtils::ReplaceMessage(format.empty() ? "%album%" : format, song);
}

std::string Totals(int songs, int artists, int albums) {
  auto line = [](int count, const char *one, const char *many) {
    return std::to_string(count) + " " + (count == 1 ? one : many);
  };
  return line(songs, "song", "songs") + "\n" + line(artists, "artist", "artists") + "\n" + line(albums, "album", "albums");
}

}  // namespace ContextTechnical
