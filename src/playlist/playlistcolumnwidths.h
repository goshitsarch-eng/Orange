#ifndef STRAWBERRY_PLAYLISTCOLUMNWIDTHS_H
#define STRAWBERRY_PLAYLISTCOLUMNWIDTHS_H

#include "playlist/playlistdelegates.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace PlaylistColumnWidths {

inline constexpr int kMinSectionSize = 30;
inline constexpr int kDefaultSectionSize = 100;
inline constexpr int kHeaderStateVersion = 2;
inline constexpr double kResizeHandlePx = 8.0;

struct State {
  int version = kHeaderStateVersion;
  bool stretch = true;
  std::map<PlaylistColumn, double> proportions;
  std::map<PlaylistColumn, int> pixels;
};

inline PlaylistColumn ColumnFromTitle(const std::string &title) {
  for (int i = 0; i < static_cast<int>(PlaylistColumn::Count); ++i) {
    const auto column = static_cast<PlaylistColumn>(i);
    if (PlaylistDelegates::ColumnTitle(column) == title) {
      return column;
    }
  }
  return PlaylistColumn::Count;
}

// Relative width each column asks for.
// These are normalised against whichever columns are actually visible, so only the ratios matter.
// They are sized against the header title and the kind of value the column holds: a column left at the
// same share as "Track" while its title reads "Album artist" can never show more than an ellipsis.
inline double DefaultProportion(const PlaylistColumn column) {
  switch (column) {
    // Free text, longest values.
    case PlaylistColumn::Title:
    case PlaylistColumn::TitleSort:
      return 0.16;
    case PlaylistColumn::Filename:
    case PlaylistColumn::URL:
      return 0.13;
    case PlaylistColumn::Artist:
    case PlaylistColumn::ArtistSort:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumSort:
      return 0.13;
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::AlbumArtistSort:
    case PlaylistColumn::Performer:
    case PlaylistColumn::PerformerSort:
    case PlaylistColumn::Composer:
    case PlaylistColumn::ComposerSort:
    case PlaylistColumn::Comment:
    case PlaylistColumn::Grouping:
      return 0.11;
    case PlaylistColumn::Genre:
    case PlaylistColumn::DateCreated:
    case PlaylistColumn::DateModified:
    case PlaylistColumn::LastPlayed:
      return 0.08;
    // Short values, but titles wide enough to need the room.
    case PlaylistColumn::Samplerate:
    case PlaylistColumn::Bitdepth:
    case PlaylistColumn::EBUR128I:
    case PlaylistColumn::EBUR128LRA:
    case PlaylistColumn::Moodbar:
    case PlaylistColumn::Rating:
      return 0.06;
    case PlaylistColumn::Length:
    case PlaylistColumn::Bitrate:
    case PlaylistColumn::Filesize:
    case PlaylistColumn::Filetype:
    case PlaylistColumn::Source:
    case PlaylistColumn::OriginalYear:
    case PlaylistColumn::InitialKey:
      return 0.05;
    // Narrow numeric columns.
    case PlaylistColumn::Track:
    case PlaylistColumn::Disc:
    case PlaylistColumn::Year:
    case PlaylistColumn::PlayCount:
    case PlaylistColumn::SkipCount:
    case PlaylistColumn::Queue:
    case PlaylistColumn::HasCUE:
    case PlaylistColumn::BPM:
    case PlaylistColumn::Mood:
      return 0.04;
    default:
      return 0.06;
  }
}

inline bool VersionSupported(const int version) { return version == kHeaderStateVersion; }

inline std::vector<double> Normalize(std::vector<double> values) {
  double sum = 0.0;
  for (double value : values) {
    sum += std::max(0.0, value);
  }
  if (values.empty()) {
    return values;
  }
  if (sum <= 0.0) {
    const double share = 1.0 / static_cast<double>(values.size());
    std::fill(values.begin(), values.end(), share);
    return values;
  }
  for (double &value : values) {
    value = std::max(0.0, value) / sum;
  }
  return values;
}

inline int PixelFromProportion(const double proportion, const int total) {
  if (total <= 0) {
    return kDefaultSectionSize;
  }
  return std::max(kMinSectionSize, static_cast<int>(std::lround(proportion * static_cast<double>(total))));
}

inline std::vector<int> Distribute(const std::vector<double> &proportions, const int total) {
  std::vector<int> pixels(proportions.size(), kMinSectionSize);
  if (proportions.empty() || total <= 0) {
    return pixels;
  }
  int used = 0;
  for (size_t i = 0; i + 1 < proportions.size(); ++i) {
    pixels[i] = PixelFromProportion(proportions[i], total);
    used += pixels[i];
  }
  pixels.back() = std::max(kMinSectionSize, total - used);
  return pixels;
}

inline bool NeighborResize(const int left_old, const int left_new, const int right_old, int *right_new) {
  if (!right_new) {
    return false;
  }
  const int right = right_old + (left_old - left_new);
  if (left_new < kMinSectionSize || right < kMinSectionSize) {
    return false;
  }
  *right_new = right;
  return true;
}

inline bool OnResizeHandle(const double x_in_column, const double column_width, const double handle = kResizeHandlePx) {
  return column_width > handle && x_in_column >= column_width - handle;
}

inline bool OnResizeHandleAbsolute(const double x, const double column_x, const double column_width, const double handle = kResizeHandlePx) {
  const double edge = column_x + column_width;
  return x >= edge - handle && x <= edge + handle;
}

inline std::string Encode(const State &state) {
  std::ostringstream out;
  out << state.version << ';' << (state.stretch ? 1 : 0) << ';';
  bool first = true;
  for (const auto &entry : state.proportions) {
    const std::string title = PlaylistDelegates::ColumnTitle(entry.first);
    if (title.empty()) {
      continue;
    }
    if (!first) {
      out << ',';
    }
    first = false;
    out << title << '=' << entry.second;
  }
  out << ';';
  first = true;
  for (const auto &entry : state.pixels) {
    const std::string title = PlaylistDelegates::ColumnTitle(entry.first);
    if (title.empty()) {
      continue;
    }
    if (!first) {
      out << ',';
    }
    first = false;
    out << title << '=' << entry.second;
  }
  return out.str();
}

inline State Decode(const std::string &blob) {
  State state;
  if (blob.empty() || blob.find(';') == std::string::npos) {
    return state;
  }
  const auto semi1 = blob.find(';');
  const auto semi2 = blob.find(';', semi1 + 1);
  if (semi2 == std::string::npos) {
    return state;
  }
  const auto semi3 = blob.find(';', semi2 + 1);
  try {
    state.version = std::stoi(blob.substr(0, semi1));
  } catch (...) {
    return State{};
  }
  if (!VersionSupported(state.version)) {
    return State{};
  }
  state.stretch = blob.substr(semi1 + 1, semi2 - semi1 - 1) == "1";
  const std::string props = semi3 == std::string::npos ? blob.substr(semi2 + 1) : blob.substr(semi2 + 1, semi3 - semi2 - 1);
  const std::string pixels = semi3 == std::string::npos ? std::string() : blob.substr(semi3 + 1);
  auto parse_map = [](const std::string &text, bool doubles) {
    std::map<PlaylistColumn, std::pair<double, int>> parsed;
    size_t start = 0;
    while (start < text.size()) {
      const size_t comma = text.find(',', start);
      const std::string part = text.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
      const auto eq = part.find('=');
      if (eq != std::string::npos) {
        const PlaylistColumn column = ColumnFromTitle(part.substr(0, eq));
        if (column != PlaylistColumn::Count) {
          try {
            if (doubles) {
              parsed[column].first = std::stod(part.substr(eq + 1));
            } else {
              parsed[column].second = std::stoi(part.substr(eq + 1));
            }
          } catch (...) {
          }
        }
      }
      if (comma == std::string::npos) {
        break;
      }
      start = comma + 1;
    }
    return parsed;
  };
  for (const auto &entry : parse_map(props, true)) {
    state.proportions[entry.first] = entry.second.first;
  }
  for (const auto &entry : parse_map(pixels, false)) {
    state.pixels[entry.first] = entry.second.second;
  }
  return state;
}

}  // namespace PlaylistColumnWidths

#endif
