#include "context/contexttechnical.h"

#include "utilities/strutils.h"
#include "utilities/timeutils.h"

namespace ContextTechnical {

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

}  // namespace ContextTechnical
