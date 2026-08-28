#include "collection/collectiongrouping.h"

#include "core/settings.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

const char *CollectionGrouping::kComboLabels[] = {
    "None",
    "Artist",
    "Album artist",
    "Album",
    "Album - Disc",
    "Disc",
    "Format",
    "Genre",
    "Year",
    "Year - Album",
    "Year - Album - Disc",
    "Original year",
    "Original year - Album",
    "Original year - Album - Disc",
    "Composer",
    "Performer",
    "Grouping",
    "File type",
    "Sample rate",
    "Bit depth",
    "Bitrate",
    nullptr};

const CollectionGrouping::GroupBy CollectionGrouping::kComboValues[] = {
    GroupBy::None,         GroupBy::Artist,       GroupBy::AlbumArtist, GroupBy::Album,      GroupBy::AlbumDisc,
    GroupBy::Disc,         GroupBy::Format,       GroupBy::Genre,       GroupBy::Year,       GroupBy::YearAlbum,
    GroupBy::YearAlbumDisc, GroupBy::OriginalYear, GroupBy::OriginalYearAlbum, GroupBy::OriginalYearAlbumDisc,
    GroupBy::Composer,     GroupBy::Performer,    GroupBy::Grouping,    GroupBy::FileType,   GroupBy::Samplerate,
    GroupBy::Bitdepth,     GroupBy::Bitrate};

int CollectionGrouping::ComboCount() { return 21; }

CollectionGrouping::GroupBy CollectionGrouping::Grouping::operator[](int i) const {
  if (i == 0) return first;
  if (i == 1) return second;
  return third;
}

bool CollectionGrouping::Grouping::operator==(const Grouping &other) const {
  return first == other.first && second == other.second && third == other.third;
}

CollectionGrouping::GroupBy CollectionGrouping::FromInt(int value) {
  if (value < 0 || value >= static_cast<int>(GroupBy::GroupByCount)) {
    return GroupBy::None;
  }
  return static_cast<GroupBy>(value);
}

int CollectionGrouping::ComboIndex(GroupBy group_by) {
  for (int i = 0; i < ComboCount(); ++i) {
    if (kComboValues[i] == group_by) {
      return i;
    }
  }
  return 0;
}

CollectionGrouping::GroupBy CollectionGrouping::FromComboIndex(int index) {
  if (index < 0 || index >= ComboCount()) {
    return GroupBy::None;
  }
  return kComboValues[index];
}

std::string CollectionGrouping::Label(GroupBy group_by) { return kComboLabels[ComboIndex(group_by)]; }

CollectionGrouping::Grouping CollectionGrouping::FromLegacy(const std::string &legacy) {
  if (legacy == "album") {
    return {GroupBy::Album, GroupBy::None, GroupBy::None};
  }
  if (legacy == "genre") {
    return {GroupBy::Genre, GroupBy::AlbumArtist, GroupBy::Album};
  }
  if (legacy == "year") {
    return {GroupBy::Year, GroupBy::AlbumArtist, GroupBy::Album};
  }
  if (legacy == "artist") {
    return {GroupBy::Artist, GroupBy::Album, GroupBy::None};
  }
  return {GroupBy::AlbumArtist, GroupBy::Album, GroupBy::None};
}

int CollectionGrouping::EffectiveOriginalYear(const Song &song) {
  return song.originalyear() > 0 ? song.originalyear() : song.year();
}

bool CollectionGrouping::AlbumContainsDisc(const std::string &album) {
  return StrUtils::ContainsInsensitive(album, "disc") || StrUtils::ContainsInsensitive(album, "cd");
}

bool CollectionGrouping::IsAlbumGroupBy(GroupBy group_by) {
  return group_by == GroupBy::Album || group_by == GroupBy::YearAlbum || group_by == GroupBy::AlbumDisc ||
         group_by == GroupBy::YearAlbumDisc || group_by == GroupBy::OriginalYearAlbum || group_by == GroupBy::OriginalYearAlbumDisc;
}

bool CollectionGrouping::IsArtistGroupBy(GroupBy group_by) {
  return group_by == GroupBy::Artist || group_by == GroupBy::AlbumArtist;
}

std::string CollectionGrouping::TextOrUnknown(const std::string &text) { return text.empty() ? "Unknown" : text; }

std::string CollectionGrouping::PrettyYearAlbum(int year, const std::string &album) {
  if (year <= 0) {
    return TextOrUnknown(album);
  }
  return std::to_string(year) + " - " + TextOrUnknown(album);
}

std::string CollectionGrouping::PrettyAlbumDisc(const std::string &album, int disc) {
  if (disc <= 0 || AlbumContainsDisc(album)) {
    return TextOrUnknown(album);
  }
  return TextOrUnknown(album) + " - (Disc " + std::to_string(disc) + ")";
}

std::string CollectionGrouping::PrettyYearAlbumDisc(int year, const std::string &album, int disc) {
  std::string text = year <= 0 ? TextOrUnknown(album) : std::to_string(year) + " - " + TextOrUnknown(album);
  if (!AlbumContainsDisc(album) && disc > 0) {
    text += " - (Disc " + std::to_string(disc) + ")";
  }
  return text;
}

std::string CollectionGrouping::PrettyDisc(int disc) { return "Disc " + std::to_string(std::max(1, disc)); }

std::string CollectionGrouping::PrettyFormat(const Song &song) {
  const std::string type = Song::FiletypeToString(song.filetype());
  if (song.samplerate() <= 0) {
    return type;
  }
  char rate[32];
  std::snprintf(rate, sizeof(rate), "%.5g", song.samplerate() / 1000.0);
  if (song.bitdepth() <= 0) {
    return type + " (" + rate + ")";
  }
  return type + " (" + rate + "/" + std::to_string(song.bitdepth()) + ")";
}

std::string CollectionGrouping::DisplayText(GroupBy group_by, const Song &song) {
  switch (group_by) {
    case GroupBy::AlbumArtist:
      return TextOrUnknown(song.EffectiveAlbumartist());
    case GroupBy::Artist:
      return TextOrUnknown(song.artist());
    case GroupBy::Album:
      return TextOrUnknown(song.album());
    case GroupBy::AlbumDisc:
      return PrettyAlbumDisc(song.album(), song.disc());
    case GroupBy::YearAlbum:
      return PrettyYearAlbum(song.year(), song.album());
    case GroupBy::YearAlbumDisc:
      return PrettyYearAlbumDisc(song.year(), song.album(), song.disc());
    case GroupBy::OriginalYearAlbum:
      return PrettyYearAlbum(EffectiveOriginalYear(song), song.album());
    case GroupBy::OriginalYearAlbumDisc:
      return PrettyYearAlbumDisc(EffectiveOriginalYear(song), song.album(), song.disc());
    case GroupBy::Disc:
      return PrettyDisc(std::max(0, song.disc()));
    case GroupBy::Year:
      return std::to_string(std::max(0, song.year()));
    case GroupBy::OriginalYear:
      return std::to_string(std::max(0, EffectiveOriginalYear(song)));
    case GroupBy::Genre:
      return TextOrUnknown(song.genre());
    case GroupBy::Composer:
      return TextOrUnknown(song.composer());
    case GroupBy::Performer:
      return TextOrUnknown(song.performer());
    case GroupBy::Grouping:
      return TextOrUnknown(song.grouping());
    case GroupBy::FileType:
      return Song::FiletypeToString(song.filetype());
    case GroupBy::Format:
      return PrettyFormat(song);
    case GroupBy::Samplerate:
      return std::to_string(std::max(0, song.samplerate()));
    case GroupBy::Bitdepth:
      return std::to_string(std::max(0, song.bitdepth()));
    case GroupBy::Bitrate:
      return std::to_string(std::max(0, song.bitrate()));
    case GroupBy::None:
    case GroupBy::GroupByCount:
      return song.PrettyTitle();
  }
  return {};
}

std::string CollectionGrouping::NormalizeSort(std::string text) {
  if (text.empty()) {
    return " unknown";
  }
  text = StrUtils::ToLower(text);
  text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) { return !std::isalnum(c) && c != ' '; }), text.end());
  return text;
}

std::string CollectionGrouping::SkipArticles(std::string name) {
  static const char *articles[] = {"the ", "a ", "an ", nullptr};
  const std::string lower = StrUtils::ToLower(name);
  for (int i = 0; articles[i]; ++i) {
    if (StrUtils::StartsWith(lower, articles[i])) {
      const size_t len = std::char_traits<char>::length(articles[i]);
      return name.substr(len) + ", " + name.substr(0, len - 1);
    }
  }
  return name;
}

std::string CollectionGrouping::SortTextForName(const std::string &name, bool skip_articles) {
  return skip_articles ? NormalizeSort(SkipArticles(name)) : NormalizeSort(name);
}

std::string CollectionGrouping::SortTextForNumber(int number) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d", std::max(0, number));
  return buf;
}

std::string CollectionGrouping::SortTextForYear(int year) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%04d", std::max(0, year));
  return buf;
}

namespace {

// A sort tag is only honoured when the file actually carries one.
const std::string &SortTagOr(const std::string &sort_tag, const std::string &name, bool use_sort_tags) {
  return use_sort_tags && !sort_tag.empty() ? sort_tag : name;
}

}  // namespace

std::string CollectionGrouping::SortText(GroupBy group_by, const Song &song, bool skip_artist_articles, bool skip_album_articles,
                                         bool use_sort_tags) {
  const std::string &album = SortTagOr(song.albumsort(), song.album(), use_sort_tags);
  switch (group_by) {
    case GroupBy::AlbumArtist:
      return SortTextForName(SortTagOr(song.albumartistsort(), song.EffectiveAlbumartist(), use_sort_tags),
                             skip_artist_articles);
    case GroupBy::Artist:
      return SortTextForName(SortTagOr(song.artistsort(), song.artist(), use_sort_tags), skip_artist_articles);
    case GroupBy::Album:
      return SortTextForName(album, skip_album_articles);
    case GroupBy::AlbumDisc:
      return SortTextForName(album, skip_album_articles) + SortTextForNumber(song.disc());
    case GroupBy::YearAlbum:
      return SortTextForYear(song.year()) + SortTextForName(album, skip_album_articles);
    case GroupBy::YearAlbumDisc:
      return SortTextForYear(song.year()) + SortTextForName(album, skip_album_articles) + SortTextForNumber(song.disc());
    case GroupBy::OriginalYearAlbum:
      return SortTextForYear(EffectiveOriginalYear(song)) + SortTextForName(album, skip_album_articles);
    case GroupBy::OriginalYearAlbumDisc:
      return SortTextForYear(EffectiveOriginalYear(song)) + SortTextForName(album, skip_album_articles) +
             SortTextForNumber(song.disc());
    case GroupBy::Disc:
      return SortTextForNumber(song.disc());
    case GroupBy::Year:
      return SortTextForYear(song.year());
    case GroupBy::OriginalYear:
      return SortTextForYear(EffectiveOriginalYear(song));
    case GroupBy::Genre:
      return NormalizeSort(song.genre());
    case GroupBy::Composer:
      return SortTextForName(SortTagOr(song.composersort(), song.composer(), use_sort_tags), skip_artist_articles);
    case GroupBy::Performer:
      return SortTextForName(song.performer(), skip_artist_articles);
    case GroupBy::Grouping:
      return NormalizeSort(song.grouping());
    case GroupBy::FileType:
      return Song::FiletypeToString(song.filetype());
    case GroupBy::Format:
      return PrettyFormat(song);
    case GroupBy::Samplerate:
      return SortTextForNumber(song.samplerate());
    case GroupBy::Bitdepth:
      return SortTextForNumber(song.bitdepth());
    case GroupBy::Bitrate:
      return SortTextForNumber(song.bitrate());
    case GroupBy::None:
    case GroupBy::GroupByCount:
      break;
  }
  return {};
}

std::string CollectionGrouping::ContainerKey(GroupBy group_by, const Song &song, bool *has_unique_album_identifier,
                                             bool separate_albums_by_grouping) {
  std::string key;
  switch (group_by) {
    case GroupBy::AlbumArtist:
      key = TextOrUnknown(song.EffectiveAlbumartist());
      if (has_unique_album_identifier) *has_unique_album_identifier = true;
      break;
    case GroupBy::Artist:
      key = TextOrUnknown(song.artist());
      if (has_unique_album_identifier) *has_unique_album_identifier = true;
      break;
    case GroupBy::Album:
      key = TextOrUnknown(song.album());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::AlbumDisc:
      key = TextOrUnknown(song.album()) + "-" + SortTextForNumber(song.disc());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::YearAlbum:
      key = SortTextForYear(song.year()) + "-" + TextOrUnknown(song.album());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::YearAlbumDisc:
      key = SortTextForYear(song.year()) + "-" + TextOrUnknown(song.album()) + "-" + SortTextForNumber(song.disc());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::OriginalYearAlbum:
      key = SortTextForYear(EffectiveOriginalYear(song)) + "-" + TextOrUnknown(song.album());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::OriginalYearAlbumDisc:
      key = SortTextForYear(EffectiveOriginalYear(song)) + "-" + TextOrUnknown(song.album()) + "-" + SortTextForNumber(song.disc());
      if (!song.album_id().empty()) key += "-" + song.album_id();
      if (separate_albums_by_grouping && !song.grouping().empty()) key += "-" + song.grouping();
      break;
    case GroupBy::Disc:
      key = PrettyDisc(song.disc());
      break;
    case GroupBy::Year:
      key = SortTextForYear(song.year());
      break;
    case GroupBy::OriginalYear:
      key = SortTextForYear(EffectiveOriginalYear(song));
      break;
    case GroupBy::Genre:
      key = TextOrUnknown(song.genre());
      break;
    case GroupBy::Composer:
      key = TextOrUnknown(song.composer());
      if (has_unique_album_identifier) *has_unique_album_identifier = true;
      break;
    case GroupBy::Performer:
      key = TextOrUnknown(song.performer());
      if (has_unique_album_identifier) *has_unique_album_identifier = true;
      break;
    case GroupBy::Grouping:
      key = TextOrUnknown(song.grouping());
      break;
    case GroupBy::FileType:
      key = Song::FiletypeToString(song.filetype());
      break;
    case GroupBy::Samplerate:
      key = std::to_string(std::max(0, song.samplerate()));
      break;
    case GroupBy::Bitdepth:
      key = std::to_string(std::max(0, song.bitdepth()));
      break;
    case GroupBy::Bitrate:
      key = std::to_string(std::max(0, song.bitrate()));
      break;
    case GroupBy::Format:
      key = PrettyFormat(song);
      break;
    case GroupBy::None:
    case GroupBy::GroupByCount:
      break;
  }
  if (IsAlbumGroupBy(group_by) && has_unique_album_identifier && !*has_unique_album_identifier && !song.compilation() &&
      !song.EffectiveAlbumartist().empty()) {
    key = TextOrUnknown(song.EffectiveAlbumartist()) + "-" + key;
    *has_unique_album_identifier = true;
  }
  return key;
}

CollectionGrouping::Node *CollectionGrouping::FindOrAddChild(Node *parent, const std::string &key, const std::string &display,
                                                             const std::string &sort) {
  for (Node &child : parent->children) {
    if (child.key == key) {
      return &child;
    }
  }
  Node child;
  child.key = key;
  child.display = display;
  child.sort = sort;
  parent->children.push_back(child);
  return &parent->children.back();
}

CollectionGrouping::Node CollectionGrouping::BuildTree(const SongList &songs, const Grouping &grouping, bool separate_albums_by_grouping,
                                                       bool skip_artist_articles, bool skip_album_articles, bool use_sort_tags) {
  Node root;
  for (const Song &song : songs) {
    Node *current = &root;
    bool unique = false;
    bool placed = false;
    for (int level = 0; level < 3; ++level) {
      const GroupBy group_by = grouping[level];
      if (group_by == GroupBy::None) {
        break;
      }
      const std::string key = ContainerKey(group_by, song, &unique, separate_albums_by_grouping);
      current = FindOrAddChild(current, key, DisplayText(group_by, song),
                               SortText(group_by, song, skip_artist_articles, skip_album_articles, use_sort_tags));
      placed = true;
    }
    current->songs.push_back(song);
    (void)placed;
  }
  auto sort_nodes = [](auto &self, Node *node) -> void {
    std::sort(node->children.begin(), node->children.end(), [](const Node &a, const Node &b) {
      if (a.sort != b.sort) {
        return a.sort < b.sort;
      }
      return a.display < b.display;
    });
    std::sort(node->songs.begin(), node->songs.end(), [](const Song &a, const Song &b) {
      if (a.disc() != b.disc()) {
        return a.disc() < b.disc();
      }
      if (a.track() != b.track()) {
        return a.track() < b.track();
      }
      return a.PrettyTitle() < b.PrettyTitle();
    });
    for (Node &child : node->children) {
      self(self, &child);
    }
  };
  sort_nodes(sort_nodes, &root);
  return root;
}

CollectionGrouping::Grouping CollectionGrouping::LoadCurrent() {
  Settings settings;
  settings.BeginGroup("Collection");
  const int first = settings.IntValue("group_by1", -1);
  if (first < 0) {
    return FromLegacy(settings.Value("groupby", "artist-album"));
  }
  return {FromInt(first), FromInt(settings.IntValue("group_by2", 0)), FromInt(settings.IntValue("group_by3", 0))};
}

void CollectionGrouping::SaveCurrent(const Grouping &grouping) {
  Settings settings;
  settings.BeginGroup("Collection");
  settings.SetIntValue("group_by1", static_cast<int>(grouping.first));
  settings.SetIntValue("group_by2", static_cast<int>(grouping.second));
  settings.SetIntValue("group_by3", static_cast<int>(grouping.third));
  settings.Sync();
}

bool CollectionGrouping::SeparateAlbumsByGrouping() {
  Settings settings;
  settings.BeginGroup("Collection");
  return settings.BoolValue("separate_albums_by_grouping", false);
}

void CollectionGrouping::SetSeparateAlbumsByGrouping(bool value) {
  Settings settings;
  settings.BeginGroup("Collection");
  settings.SetBoolValue("separate_albums_by_grouping", value);
  settings.Sync();
}

namespace {

std::string EncodeGrouping(const CollectionGrouping::Grouping &grouping) {
  return std::to_string(static_cast<int>(grouping.first)) + "," + std::to_string(static_cast<int>(grouping.second)) + "," +
         std::to_string(static_cast<int>(grouping.third));
}

CollectionGrouping::Grouping DecodeGrouping(const std::string &text) {
  CollectionGrouping::Grouping grouping;
  int values[3] = {0, 0, 0};
  std::sscanf(text.c_str(), "%d,%d,%d", &values[0], &values[1], &values[2]);
  grouping.first = CollectionGrouping::FromInt(values[0]);
  grouping.second = CollectionGrouping::FromInt(values[1]);
  grouping.third = CollectionGrouping::FromInt(values[2]);
  return grouping;
}

}  // namespace

std::vector<std::pair<std::string, CollectionGrouping::Grouping>> CollectionGrouping::LoadSaved() {
  Settings settings;
  settings.BeginGroup("SavedGroupings");
  std::vector<std::pair<std::string, Grouping>> result;
  const std::string blob = settings.Value("presets");
  for (const std::string &line : StrUtils::Split(blob, '|')) {
    const auto tab = line.find('\t');
    if (tab == std::string::npos) {
      continue;
    }
    result.emplace_back(line.substr(0, tab), DecodeGrouping(line.substr(tab + 1)));
  }
  return result;
}

void CollectionGrouping::SaveAll(const std::vector<std::pair<std::string, Grouping>> &saved) {
  std::string blob;
  for (size_t i = 0; i < saved.size(); ++i) {
    if (i > 0) {
      blob += "|";
    }
    blob += saved[i].first + "\t" + EncodeGrouping(saved[i].second);
  }
  Settings settings;
  settings.BeginGroup("SavedGroupings");
  settings.SetValue("presets", blob);
  settings.Sync();
}

void CollectionGrouping::AddSaved(const std::string &name, const Grouping &grouping) {
  if (name.empty()) {
    return;
  }
  auto saved = LoadSaved();
  for (auto &entry : saved) {
    if (entry.first == name) {
      entry.second = grouping;
      SaveAll(saved);
      return;
    }
  }
  saved.emplace_back(name, grouping);
  SaveAll(saved);
}

void CollectionGrouping::RemoveSaved(const std::string &name) {
  auto saved = LoadSaved();
  saved.erase(std::remove_if(saved.begin(), saved.end(), [&](const auto &entry) { return entry.first == name; }), saved.end());
  SaveAll(saved);
}
