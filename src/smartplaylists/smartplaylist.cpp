#include "smartplaylists/smartplaylist.h"

#include "collection/collectionbackend.h"
#include "core/settings.h"
#include "smartplaylists/smartplaylistdateunits.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>
#include <sstream>

namespace {

std::string FieldText(const Song &song, SmartPlaylistField field) {
  switch (field) {
    case SmartPlaylistField::Title:
      return song.title();
    case SmartPlaylistField::Album:
      return song.album();
    case SmartPlaylistField::Artist:
      return song.artist();
    case SmartPlaylistField::AlbumArtist:
      return song.EffectiveAlbumartist();
    case SmartPlaylistField::Composer:
      return song.composer();
    case SmartPlaylistField::Genre:
      return song.genre();
    case SmartPlaylistField::Performer:
      return song.performer();
    case SmartPlaylistField::Grouping:
      return song.grouping();
    case SmartPlaylistField::Comment:
      return song.comment();
    case SmartPlaylistField::Filepath:
      return FileUtils::PathFromUri(song.url());
    case SmartPlaylistField::Filetype:
      return Song::FiletypeToString(song.filetype());
    case SmartPlaylistField::Mood:
      return song.mood();
    case SmartPlaylistField::InitialKey:
      return song.initial_key();
    default:
      return {};
  }
}

double FieldNumber(const Song &song, SmartPlaylistField field) {
  switch (field) {
    case SmartPlaylistField::Year:
      return song.year();
    case SmartPlaylistField::OriginalYear:
      return song.originalyear();
    case SmartPlaylistField::Rating:
      return song.rating();
    case SmartPlaylistField::Playcount:
      return song.playcount();
    case SmartPlaylistField::Skipcount:
      return song.skipcount();
    case SmartPlaylistField::Length:
      return static_cast<double>(song.length_nanosec());
    case SmartPlaylistField::Bitrate:
      return song.bitrate();
    case SmartPlaylistField::LastPlayed:
      return static_cast<double>(song.lastplayed());
    case SmartPlaylistField::Track:
      return song.track();
    case SmartPlaylistField::Disc:
      return song.disc();
    case SmartPlaylistField::Filesize:
      return static_cast<double>(song.filesize());
    case SmartPlaylistField::DateCreated:
      return static_cast<double>(song.ctime());
    case SmartPlaylistField::DateModified:
      return static_cast<double>(song.mtime());
    case SmartPlaylistField::Samplerate:
      return song.samplerate();
    case SmartPlaylistField::Bitdepth:
      return song.bitdepth();
    case SmartPlaylistField::BPM:
      return song.bpm();
    default:
      return 0;
  }
}

}  // namespace

int64_t SmartPlaylistSearch::ParseDateValue(const std::string &value) {
  int year = 0;
  int month = 0;
  int day = 0;
  if (std::sscanf(value.c_str(), "%d-%d-%d", &year, &month, &day) == 3 && year > 1900 && month >= 1 && month <= 12 && day >= 1 &&
      day <= 31) {
    GDateTime *dt = g_date_time_new_utc(year, month, day, 0, 0, 0);
    if (dt) {
      const int64_t unix_time = g_date_time_to_unix(dt);
      g_date_time_unref(dt);
      return unix_time;
    }
  }
  return std::strtoll(value.c_str(), nullptr, 10);
}

SmartPlaylistFieldKind SmartPlaylistSearch::KindOf(SmartPlaylistField field) {
  switch (field) {
    case SmartPlaylistField::Year:
    case SmartPlaylistField::OriginalYear:
    case SmartPlaylistField::Playcount:
    case SmartPlaylistField::Skipcount:
    case SmartPlaylistField::Bitrate:
    case SmartPlaylistField::Track:
    case SmartPlaylistField::Disc:
    case SmartPlaylistField::Filesize:
    case SmartPlaylistField::Samplerate:
    case SmartPlaylistField::Bitdepth:
    case SmartPlaylistField::BPM:
      return SmartPlaylistFieldKind::Number;
    case SmartPlaylistField::Rating:
      return SmartPlaylistFieldKind::Rating;
    case SmartPlaylistField::Length:
      return SmartPlaylistFieldKind::Time;
    case SmartPlaylistField::DateCreated:
    case SmartPlaylistField::LastPlayed:
    case SmartPlaylistField::DateModified:
      return SmartPlaylistFieldKind::Date;
    default:
      return SmartPlaylistFieldKind::Text;
  }
}

const char *SmartPlaylistSearch::TypeName(SearchType type) {
  switch (type) {
    case SearchType::Or:
      return "Or";
    case SearchType::All:
      return "All";
    case SearchType::And:
    default:
      return "And";
  }
}

SmartPlaylistSearch::SearchType SmartPlaylistSearch::TypeFromName(const std::string &name) {
  if (name == "Or") {
    return SearchType::Or;
  }
  if (name == "All") {
    return SearchType::All;
  }
  return SearchType::And;
}

bool SmartPlaylistSearch::TermsApply(SearchType type) { return type != SearchType::All; }

std::vector<SmartPlaylistOp> SmartPlaylistSearch::OperatorsFor(SmartPlaylistField field) {
  switch (KindOf(field)) {
    case SmartPlaylistFieldKind::Number:
    case SmartPlaylistFieldKind::Rating:
    case SmartPlaylistFieldKind::Time:
      return {SmartPlaylistOp::Equals, SmartPlaylistOp::NotEquals, SmartPlaylistOp::GreaterThan, SmartPlaylistOp::LessThan,
              SmartPlaylistOp::Empty, SmartPlaylistOp::NotEmpty};
    case SmartPlaylistFieldKind::Date:
      return {SmartPlaylistOp::NumericDate, SmartPlaylistOp::RelativeDate, SmartPlaylistOp::NumericDateNot, SmartPlaylistOp::RelativeRange,
              SmartPlaylistOp::GreaterThan, SmartPlaylistOp::LessThan, SmartPlaylistOp::Empty, SmartPlaylistOp::NotEmpty};
    case SmartPlaylistFieldKind::Text:
    default:
      return {SmartPlaylistOp::Contains, SmartPlaylistOp::NotContains, SmartPlaylistOp::Equals, SmartPlaylistOp::NotEquals,
              SmartPlaylistOp::StartsWith, SmartPlaylistOp::EndsWith, SmartPlaylistOp::Empty, SmartPlaylistOp::NotEmpty};
  }
}

std::string SmartPlaylistSearch::OpName(SmartPlaylistOp op) {
  switch (op) {
    case SmartPlaylistOp::Contains:
      return "Contains";
    case SmartPlaylistOp::NotContains:
      return "Not contains";
    case SmartPlaylistOp::Equals:
      return "Equals";
    case SmartPlaylistOp::GreaterThan:
      return "Greater than";
    case SmartPlaylistOp::LessThan:
      return "Less than";
    case SmartPlaylistOp::StartsWith:
      return "Starts with";
    case SmartPlaylistOp::EndsWith:
      return "Ends with";
    case SmartPlaylistOp::NotEquals:
      return "Not equals";
    case SmartPlaylistOp::Empty:
      return "Empty";
    case SmartPlaylistOp::NotEmpty:
      return "Not empty";
    case SmartPlaylistOp::NumericDate:
      return "On date";
    case SmartPlaylistOp::RelativeDate:
      return "In the last";
    case SmartPlaylistOp::NumericDateNot:
      return "Not in the last";
    case SmartPlaylistOp::RelativeRange:
      return "Between";
  }
  return "Contains";
}

bool SmartPlaylistTerm::Matches(const Song &song) const {
  const std::string text = FieldText(song, field);
  double number = FieldNumber(song, field);
  double wanted = std::strtod(value.c_str(), nullptr);
  if (SmartPlaylistSearch::KindOf(field) == SmartPlaylistFieldKind::Time && wanted > 0 && wanted < 1000000.0) {
    wanted *= 1000000000.0;
  }
  if (SmartPlaylistSearch::KindOf(field) == SmartPlaylistFieldKind::Date &&
      (op == SmartPlaylistOp::GreaterThan || op == SmartPlaylistOp::LessThan)) {
    wanted = static_cast<double>(SmartPlaylistSearch::ParseDateValue(value));
  }
  switch (op) {
    case SmartPlaylistOp::Contains:
      return StrUtils::ContainsInsensitive(text, value);
    case SmartPlaylistOp::NotContains:
      return !StrUtils::ContainsInsensitive(text, value);
    case SmartPlaylistOp::Equals:
      return StrUtils::ToLower(text) == StrUtils::ToLower(value) || number == wanted;
    case SmartPlaylistOp::NotEquals:
      return StrUtils::ToLower(text) != StrUtils::ToLower(value) && number != wanted;
    case SmartPlaylistOp::GreaterThan:
      return number > wanted;
    case SmartPlaylistOp::LessThan:
      return number < wanted;
    case SmartPlaylistOp::StartsWith:
      return StrUtils::StartsWith(StrUtils::ToLower(text), StrUtils::ToLower(value));
    case SmartPlaylistOp::EndsWith:
      return StrUtils::EndsWith(StrUtils::ToLower(text), StrUtils::ToLower(value));
    case SmartPlaylistOp::Empty:
      return text.empty() && number <= 0;
    case SmartPlaylistOp::NotEmpty:
      return !text.empty() || number > 0;
    case SmartPlaylistOp::NumericDate: {
      const int64_t wanted_day = SmartPlaylistSearch::ParseDateValue(value);
      const int64_t have = static_cast<int64_t>(number);
      return have >= wanted_day && have < wanted_day + 86400;
    }
    case SmartPlaylistOp::RelativeDate: {
      const int64_t count = std::max<int64_t>(0, static_cast<int64_t>(wanted));
      return SmartPlaylistDateUnits::InTheLast(static_cast<int64_t>(number), date_type, count);
    }
    case SmartPlaylistOp::NumericDateNot: {
      const int64_t count = std::max<int64_t>(0, static_cast<int64_t>(wanted));
      return SmartPlaylistDateUnits::NotInTheLast(static_cast<int64_t>(number), date_type, count);
    }
    case SmartPlaylistOp::RelativeRange: {
      const int64_t first = std::max<int64_t>(0, static_cast<int64_t>(wanted));
      const int64_t second = std::max<int64_t>(0, static_cast<int64_t>(std::strtod(second_value.c_str(), nullptr)));
      return SmartPlaylistDateUnits::Between(static_cast<int64_t>(number), date_type, first, second);
    }
  }
  return false;
}

bool SmartPlaylistTerm::IsValid() const {
  if (op == SmartPlaylistOp::Empty || op == SmartPlaylistOp::NotEmpty) {
    return true;
  }
  const SmartPlaylistFieldKind kind = SmartPlaylistSearch::KindOf(field);
  if (op == SmartPlaylistOp::RelativeDate || op == SmartPlaylistOp::NumericDateNot) {
    return std::strtoll(value.c_str(), nullptr, 10) >= 0;
  }
  if (op == SmartPlaylistOp::RelativeRange) {
    const int64_t first = std::strtoll(value.c_str(), nullptr, 10);
    const int64_t second = std::strtoll(second_value.c_str(), nullptr, 10);
    return first >= 0 && first < second;
  }
  if (kind == SmartPlaylistFieldKind::Date || op == SmartPlaylistOp::NumericDate) {
    return SmartPlaylistSearch::ParseDateValue(value) != 0;
  }
  switch (kind) {
    case SmartPlaylistFieldKind::Text:
      return !value.empty();
    case SmartPlaylistFieldKind::Number:
      return std::strtod(value.c_str(), nullptr) >= 0;
    case SmartPlaylistFieldKind::Time:
      return true;
    case SmartPlaylistFieldKind::Rating:
      return std::strtod(value.c_str(), nullptr) >= 0.0;
    case SmartPlaylistFieldKind::Date:
      return SmartPlaylistSearch::ParseDateValue(value) != 0;
  }
  return false;
}

SongList SmartPlaylistSearch::Search(const SongList &songs) const {
  SongList result;
  for (const Song &song : songs) {
    bool matches = type == SearchType::All || terms.empty();
    if (!matches) {
      matches = type == SearchType::And;
      for (const SmartPlaylistTerm &term : terms) {
        const bool term_matches = term.Matches(song);
        if (type == SearchType::And) {
          matches = matches && term_matches;
        } else {
          matches = matches || term_matches;
        }
      }
    }
    if (matches) {
      result.push_back(song);
    }
  }
  if (sort_random) {
    std::mt19937 rng{std::random_device{}()};
    std::shuffle(result.begin(), result.end(), rng);
  } else {
    std::sort(result.begin(), result.end(), [this](const Song &a, const Song &b) {
      const std::string ta = FieldText(a, sort_field);
      const std::string tb = FieldText(b, sort_field);
      if (!ta.empty() || !tb.empty()) {
        return sort_descending ? ta > tb : ta < tb;
      }
      const double na = FieldNumber(a, sort_field);
      const double nb = FieldNumber(b, sort_field);
      return sort_descending ? na > nb : na < nb;
    });
  }
  if (first_item > 0 && first_item < static_cast<int>(result.size())) {
    result.erase(result.begin(), result.begin() + first_item);
  } else if (first_item >= static_cast<int>(result.size())) {
    result.clear();
  }
  if (!id_not_in.empty()) {
    result.erase(std::remove_if(result.begin(), result.end(),
                                [this](const Song &song) {
                                  return song.id() > 0 &&
                                         std::find(id_not_in.begin(), id_not_in.end(), song.id()) != id_not_in.end();
                                }),
                 result.end());
  }
  if (limit > 0 && static_cast<int>(result.size()) > limit) {
    result.resize(static_cast<size_t>(limit));
  }
  return result;
}

SongList SmartPlaylistSearch::Search(CollectionBackend *backend) const {
  if (!backend) {
    return {};
  }
  return Search(backend->Songs());
}

std::vector<std::string> SmartPlaylistSearch::FieldNames() {
  return {"Title",     "Album",         "Artist",     "Album artist", "Composer", "Genre",      "Year",
          "Rating",    "Play count",    "Skip count", "Length",       "Bitrate",  "Date created", "Last played",
          "Track",     "Disc",          "Original year", "Performer", "Grouping", "Comment",    "File path",
          "File type", "File size",     "Date modified", "Sample rate", "Bit depth", "BPM",     "Mood",
          "Initial key"};
}

std::vector<std::string> SmartPlaylistSearch::OpNames() {
  return {"Contains", "Not contains", "Equals", "Greater than", "Less than", "Starts with", "Ends with", "Not equals", "Empty",
          "Not empty", "On date", "In the last", "Not in the last", "Between"};
}

SmartPlaylistField SmartPlaylistSearch::FieldFromIndex(int index) {
  if (index < 0 || index >= static_cast<int>(FieldNames().size())) {
    return SmartPlaylistField::Title;
  }
  return static_cast<SmartPlaylistField>(index);
}

SmartPlaylistOp SmartPlaylistSearch::OpFromIndex(int index) {
  if (index < 0 || index >= static_cast<int>(OpNames().size())) {
    return SmartPlaylistOp::Contains;
  }
  return static_cast<SmartPlaylistOp>(index);
}

namespace {

std::string SanitizeToken(std::string value) {
  for (char &ch : value) {
    if (ch == ';' || ch == '|' || ch == '\t') {
      ch = ' ';
    }
  }
  return value;
}

}  // namespace

std::string SmartPlaylistSearch::Serialize() const {
  std::ostringstream out;
  out << TypeName(type) << ',' << limit << ',' << static_cast<int>(sort_field) << ',' << (sort_descending ? 1 : 0);
  if (sort_random) {
    out << ",Random";
  }
  for (const SmartPlaylistTerm &term : terms) {
    out << ';' << static_cast<int>(term.field) << ',' << static_cast<int>(term.op) << ',' << SanitizeToken(term.value);
    if (!term.second_value.empty() || term.date_type != SmartPlaylistDateType::Day) {
      out << ',' << SanitizeToken(term.second_value) << ',' << static_cast<int>(term.date_type);
    }
  }
  return out.str();
}

bool SmartPlaylistSearch::Parse(const std::string &blob, SmartPlaylistSearch *search) {
  if (!search || blob.empty()) {
    return false;
  }
  *search = SmartPlaylistSearch();
  const auto parts = StrUtils::Split(blob, ';');
  if (parts.empty()) {
    return false;
  }
  const auto header = StrUtils::Split(parts[0], ',');
  if (header.size() < 4) {
    return false;
  }
  search->type = TypeFromName(header[0]);
  search->limit = std::atoi(header[1].c_str());
  search->sort_field = FieldFromIndex(std::atoi(header[2].c_str()));
  search->sort_descending = header[3] == "1";
  search->sort_random = header.size() >= 5 && header[4] == "Random";
  for (size_t i = 1; i < parts.size(); ++i) {
    const auto term = StrUtils::Split(parts[i], ',');
    if (term.size() < 3) {
      continue;
    }
    SmartPlaylistTerm parsed;
    parsed.field = FieldFromIndex(std::atoi(term[0].c_str()));
    parsed.op = OpFromIndex(std::atoi(term[1].c_str()));
    parsed.value = term[2];
    if (term.size() >= 5) {
      parsed.second_value = term[3];
      parsed.date_type = SmartPlaylistDateUnits::FromIndex(std::atoi(term[4].c_str()));
    } else {
      for (size_t extra = 3; extra < term.size(); ++extra) {
        parsed.value += "," + term[extra];
      }
    }
    search->terms.push_back(parsed);
  }
  return true;
}

std::vector<std::pair<std::string, SmartPlaylistSearch>> SmartPlaylistSearch::LoadSaved() {
  Settings settings;
  settings.BeginGroup("SmartPlaylists");
  std::vector<std::pair<std::string, SmartPlaylistSearch>> result;
  for (const std::string &preset : StrUtils::Split(settings.Value("presets"), '|')) {
    const auto tab = preset.find('\t');
    if (tab == std::string::npos) {
      continue;
    }
    SmartPlaylistSearch search;
    if (Parse(preset.substr(tab + 1), &search)) {
      result.emplace_back(preset.substr(0, tab), search);
    }
  }
  return result;
}

void SmartPlaylistSearch::SaveAll(const std::vector<std::pair<std::string, SmartPlaylistSearch>> &presets) {
  std::string blob;
  for (const auto &preset : presets) {
    if (!blob.empty()) {
      blob += "|";
    }
    blob += SanitizeToken(preset.first) + "\t" + preset.second.Serialize();
  }
  Settings settings;
  settings.BeginGroup("SmartPlaylists");
  settings.SetValue("presets", blob);
  settings.Sync();
}

void SmartPlaylistSearch::AddSaved(const std::string &name, const SmartPlaylistSearch &search) {
  if (name.empty()) {
    return;
  }
  auto presets = LoadSaved();
  bool replaced = false;
  for (auto &preset : presets) {
    if (preset.first == name) {
      preset.second = search;
      replaced = true;
      break;
    }
  }
  if (!replaced) {
    presets.emplace_back(name, search);
  }
  SaveAll(presets);
}

void SmartPlaylistSearch::RemoveSaved(const std::string &name) {
  auto presets = LoadSaved();
  presets.erase(std::remove_if(presets.begin(), presets.end(), [&](const auto &preset) { return preset.first == name; }), presets.end());
  SaveAll(presets);
}

bool SmartPlaylistSearch::FindSaved(const std::string &name, SmartPlaylistSearch *search) {
  if (!search) {
    return false;
  }
  for (const auto &preset : LoadSaved()) {
    if (preset.first == name) {
      *search = preset.second;
      return true;
    }
  }
  return false;
}

void SmartPlaylistSearch::RenameSaved(const std::string &old_name, const std::string &new_name, const SmartPlaylistSearch &search) {
  if (new_name.empty()) {
    return;
  }
  auto presets = LoadSaved();
  bool replaced = false;
  for (auto &preset : presets) {
    if (preset.first == old_name || preset.first == new_name) {
      if (!replaced) {
        preset.first = new_name;
        preset.second = search;
        replaced = true;
      } else {
        preset.first.clear();
      }
    }
  }
  presets.erase(std::remove_if(presets.begin(), presets.end(), [](const auto &preset) { return preset.first.empty(); }), presets.end());
  if (!replaced) {
    presets.emplace_back(new_name, search);
  }
  SaveAll(presets);
}
