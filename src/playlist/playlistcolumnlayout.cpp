#include "config.h"

#include "playlist/playlistcolumnlayout.h"

#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "playlist/playlistcolumnwidths.h"
#include "playlist/playlistheaderreorder.h"
#include "playlist/playlistmoodcolumn.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <utility>

namespace {

using PlaylistSettings::kColumnAlignments;
using PlaylistSettings::kColumns;
using PlaylistSettings::kDefaultColumns;
using PlaylistSettings::kDefaultRatingLocked;
using PlaylistSettings::kDefaultStretchColumns;
using PlaylistSettings::kRatingLocked;
using PlaylistSettings::kSettingsGroup;
using PlaylistSettings::kState;
using PlaylistSettings::kStateVersion;
using PlaylistSettings::kStretchColumns;

std::string AlignName(PlaylistColumnAlign align) {
  switch (align) {
    case PlaylistColumnAlign::Center:
      return "center";
    case PlaylistColumnAlign::Right:
      return "right";
    case PlaylistColumnAlign::Left:
      break;
  }
  return "left";
}

PlaylistColumnAlign AlignFromName(const std::string &name) {
  if (name == "center") {
    return PlaylistColumnAlign::Center;
  }
  if (name == "right") {
    return PlaylistColumnAlign::Right;
  }
  return PlaylistColumnAlign::Left;
}

std::string JoinColumns(const std::vector<PlaylistColumn> &columns) {
  std::string value;
  for (PlaylistColumn column : columns) {
    const std::string title = PlaylistDelegates::ColumnTitle(column);
    if (title.empty()) {
      continue;
    }
    if (!value.empty()) {
      value += ",";
    }
    value += title;
  }
  return value;
}

}  // namespace

std::vector<PlaylistColumn> PlaylistColumnLayout::DefaultVisible() {
  std::vector<PlaylistColumn> columns;
  for (const std::string &part : StrUtils::Split(kDefaultColumns, ',')) {
    const PlaylistColumn column = FromTitle(part);
    if (column != PlaylistColumn::Count && PlaylistMoodColumn::ShouldOffer(column)) {
      columns.push_back(column);
    }
  }
  return columns;
}

std::vector<PlaylistColumn> PlaylistColumnLayout::Visible() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  const std::string enabled = settings.Value(kColumns, kDefaultColumns);
  std::vector<PlaylistColumn> columns;
  for (const std::string &part : StrUtils::Split(enabled, ',')) {
    const PlaylistColumn column = FromTitle(part);
    if (column != PlaylistColumn::Count && PlaylistMoodColumn::ShouldOffer(column) &&
        std::find(columns.begin(), columns.end(), column) == columns.end()) {
      columns.push_back(column);
    }
  }
  if (columns.empty()) {
    return DefaultVisible();
  }
  return columns;
}

bool PlaylistColumnLayout::IsVisible(PlaylistColumn column) {
  const auto columns = Visible();
  return std::find(columns.begin(), columns.end(), column) != columns.end();
}

void PlaylistColumnLayout::SetVisibleColumns(const std::vector<PlaylistColumn> &columns) {
  std::vector<PlaylistColumn> unique;
  for (PlaylistColumn column : columns) {
    if (column == PlaylistColumn::Count || PlaylistDelegates::ColumnTitle(column).empty() ||
        !PlaylistMoodColumn::ShouldOffer(column)) {
      continue;
    }
    if (std::find(unique.begin(), unique.end(), column) == unique.end()) {
      unique.push_back(column);
    }
  }
  if (unique.empty()) {
    unique = DefaultVisible();
  }
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetValue(kColumns, JoinColumns(unique));
  settings.Sync();
}

void PlaylistColumnLayout::ToggleVisible(PlaylistColumn column) {
  auto columns = Visible();
  auto it = std::find(columns.begin(), columns.end(), column);
  if (it == columns.end()) {
    columns.push_back(column);
  } else if (columns.size() > 1) {
    columns.erase(it);
  }
  SetVisibleColumns(columns);
}

bool PlaylistColumnLayout::CanHide() { return Visible().size() > 1; }

void PlaylistColumnLayout::Hide(PlaylistColumn column) {
  auto columns = Visible();
  if (columns.size() <= 1) {
    return;
  }
  columns.erase(std::remove(columns.begin(), columns.end(), column), columns.end());
  SetVisibleColumns(columns);
}

bool PlaylistColumnLayout::Move(PlaylistColumn column, int delta) {
  if (delta == 0) {
    return false;
  }
  auto columns = Visible();
  auto it = std::find(columns.begin(), columns.end(), column);
  if (it == columns.end()) {
    return false;
  }
  const int index = static_cast<int>(std::distance(columns.begin(), it));
  const int next = index + delta;
  if (next < 0 || next >= static_cast<int>(columns.size())) {
    return false;
  }
  std::swap(columns[static_cast<size_t>(index)], columns[static_cast<size_t>(next)]);
  SetVisibleColumns(columns);
  return true;
}

bool PlaylistColumnLayout::MoveTo(PlaylistColumn column, int dest_visual) {
  const auto columns = Visible();
  const auto next = PlaylistHeaderReorder::OrderAfterMove(columns, column, dest_visual);
  if (next == columns) {
    return false;
  }
  SetVisibleColumns(next);
  return true;
}

PlaylistColumnAlign PlaylistColumnLayout::DefaultAlignment(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Year:
    case PlaylistColumn::OriginalYear:
    case PlaylistColumn::Track:
    case PlaylistColumn::Disc:
    case PlaylistColumn::Length:
    case PlaylistColumn::Samplerate:
    case PlaylistColumn::Bitdepth:
    case PlaylistColumn::Bitrate:
    case PlaylistColumn::Filesize:
    case PlaylistColumn::PlayCount:
    case PlaylistColumn::SkipCount:
    case PlaylistColumn::BPM:
    case PlaylistColumn::Queue:
      return PlaylistColumnAlign::Right;
    default:
      return PlaylistColumnAlign::Left;
  }
}

PlaylistColumnAlign PlaylistColumnLayout::Alignment(PlaylistColumn column) {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  const std::string stored = settings.Value(kColumnAlignments);
  for (const std::string &part : StrUtils::Split(stored, ',')) {
    const auto sep = part.find('=');
    if (sep == std::string::npos) {
      continue;
    }
    if (FromTitle(part.substr(0, sep)) == column) {
      return AlignFromName(part.substr(sep + 1));
    }
  }
  return DefaultAlignment(column);
}

void PlaylistColumnLayout::SetAlignment(PlaylistColumn column, PlaylistColumnAlign align) {
  std::vector<std::pair<PlaylistColumn, PlaylistColumnAlign>> values;
  bool replaced = false;
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  for (const std::string &part : StrUtils::Split(settings.Value(kColumnAlignments), ',')) {
    const auto sep = part.find('=');
    if (sep == std::string::npos) {
      continue;
    }
    const PlaylistColumn parsed = FromTitle(part.substr(0, sep));
    if (parsed == PlaylistColumn::Count) {
      continue;
    }
    const PlaylistColumnAlign value = parsed == column ? align : AlignFromName(part.substr(sep + 1));
    values.emplace_back(parsed, value);
    if (parsed == column) {
      replaced = true;
    }
  }
  if (!replaced) {
    values.emplace_back(column, align);
  }
  std::string blob;
  for (const auto &entry : values) {
    if (!blob.empty()) {
      blob += ",";
    }
    blob += PlaylistDelegates::ColumnTitle(entry.first) + "=" + AlignName(entry.second);
  }
  settings.SetValue(kColumnAlignments, blob);
  settings.Sync();
}

float PlaylistColumnLayout::XAlign(PlaylistColumn column) {
  switch (Alignment(column)) {
    case PlaylistColumnAlign::Center:
      return 0.5f;
    case PlaylistColumnAlign::Right:
      return 1.0f;
    case PlaylistColumnAlign::Left:
      break;
  }
  return 0.0f;
}

bool PlaylistColumnLayout::StretchEnabled() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  return settings.BoolValue(kStretchColumns, kDefaultStretchColumns);
}

void PlaylistColumnLayout::SetStretchEnabled(bool enabled) {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetBoolValue(kStretchColumns, enabled);
  PlaylistColumnWidths::State state = PlaylistColumnWidths::Decode(settings.Value(kState));
  if (!PlaylistColumnWidths::VersionSupported(state.version) || settings.Value(kState).empty()) {
    state = PlaylistColumnWidths::State{};
  }
  state.stretch = enabled;
  settings.SetIntValue(kStateVersion, PlaylistColumnWidths::kHeaderStateVersion);
  settings.SetValue(kState, PlaylistColumnWidths::Encode(state));
  settings.Sync();
}

bool PlaylistColumnLayout::StretchColumn(PlaylistColumn column) {
  if (!StretchEnabled()) {
    return false;
  }
  const auto columns = Visible();
  for (PlaylistColumn candidate : {PlaylistColumn::Title, PlaylistColumn::TitleSort, PlaylistColumn::Artist, PlaylistColumn::Album}) {
    if (std::find(columns.begin(), columns.end(), candidate) != columns.end()) {
      return candidate == column;
    }
  }
  return !columns.empty() && columns.front() == column;
}

namespace {

PlaylistColumnWidths::State LoadWidthState() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  if (settings.IntValue(kStateVersion, PlaylistSettings::kDefaultStateVersion) != PlaylistColumnWidths::kHeaderStateVersion) {
    return PlaylistColumnWidths::State{};
  }
  return PlaylistColumnWidths::Decode(settings.Value(kState));
}

void SaveWidthState(PlaylistColumnWidths::State state) {
  state.version = PlaylistColumnWidths::kHeaderStateVersion;
  state.stretch = PlaylistColumnLayout::StretchEnabled();
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetIntValue(kStateVersion, PlaylistColumnWidths::kHeaderStateVersion);
  settings.SetValue(kState, PlaylistColumnWidths::Encode(state));
  settings.Sync();
}

}  // namespace

std::vector<int> PlaylistColumnLayout::PixelWidths(int total_width) {
  const auto columns = Visible();
  const PlaylistColumnWidths::State state = LoadWidthState();
  if (!StretchEnabled() || total_width <= 0) {
    std::vector<int> pixels;
    pixels.reserve(columns.size());
    for (PlaylistColumn column : columns) {
      const auto it = state.pixels.find(column);
      pixels.push_back(it != state.pixels.end() ? std::max(PlaylistColumnWidths::kMinSectionSize, it->second)
                                                : PlaylistDelegates::ColumnWidth(column));
    }
    return pixels;
  }
  std::vector<double> proportions;
  proportions.reserve(columns.size());
  for (PlaylistColumn column : columns) {
    const auto it = state.proportions.find(column);
    proportions.push_back(it != state.proportions.end() ? it->second : PlaylistColumnWidths::DefaultProportion(column));
  }
  return PlaylistColumnWidths::Distribute(PlaylistColumnWidths::Normalize(proportions), total_width);
}

int PlaylistColumnLayout::PixelWidth(PlaylistColumn column, int total_width) {
  const auto columns = Visible();
  const auto widths = PixelWidths(total_width);
  for (size_t i = 0; i < columns.size() && i < widths.size(); ++i) {
    if (columns[i] == column) {
      return widths[i];
    }
  }
  return PlaylistDelegates::ColumnWidth(column);
}

void PlaylistColumnLayout::ResizePair(PlaylistColumn left, int left_px, PlaylistColumn right, int right_px, int total_width) {
  PlaylistColumnWidths::State state = LoadWidthState();
  state.pixels[left] = std::max(PlaylistColumnWidths::kMinSectionSize, left_px);
  state.pixels[right] = std::max(PlaylistColumnWidths::kMinSectionSize, right_px);
  if (total_width > 0) {
    state.proportions[left] = static_cast<double>(left_px) / static_cast<double>(total_width);
    state.proportions[right] = static_cast<double>(right_px) / static_cast<double>(total_width);
  }
  SaveWidthState(state);
}

bool PlaylistColumnLayout::RatingLocked() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  return settings.BoolValue(kRatingLocked, kDefaultRatingLocked);
}

void PlaylistColumnLayout::SetRatingLocked(bool locked) {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetBoolValue(kRatingLocked, locked);
  settings.Sync();
}

void PlaylistColumnLayout::Reset() {
  Settings settings;
  settings.BeginGroup(kSettingsGroup);
  settings.SetValue(kColumns, kDefaultColumns);
  settings.Remove(kColumnAlignments);
  settings.Remove(kState);
  settings.Remove(kStateVersion);
  settings.SetBoolValue(kStretchColumns, kDefaultStretchColumns);
  settings.SetBoolValue(kRatingLocked, kDefaultRatingLocked);
  settings.Sync();
}

PlaylistColumn PlaylistColumnLayout::FromTitle(const std::string &title) {
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const auto column = static_cast<PlaylistColumn>(i);
    if (PlaylistDelegates::ColumnTitle(column) == title) {
      return column;
    }
  }
  return PlaylistColumn::Count;
}
