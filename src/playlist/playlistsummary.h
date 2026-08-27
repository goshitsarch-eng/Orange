#ifndef STRAWBERRY_PLAYLISTSUMMARY_H
#define STRAWBERRY_PLAYLISTSUMMARY_H

#include "core/song.h"
#include "utilities/timeutils.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PlaylistSummary {

struct Input {
  int total_tracks = 0;
  int selected_tracks = 0;
  int64_t total_length_ns = 0;
  int64_t selected_length_ns = 0;
};

inline int64_t SelectedLengthNs(const SongList &songs, const std::vector<int> &rows) {
  int64_t total = 0;
  for (int row : rows) {
    if (row < 0 || row >= static_cast<int>(songs.size())) {
      continue;
    }
    if (songs[static_cast<size_t>(row)].length_nanosec() > 0) {
      total += songs[static_cast<size_t>(row)].length_nanosec();
    }
  }
  return total;
}

inline Input FromPlaylist(int total_tracks, int64_t total_length_ns, const SongList &songs, const std::vector<int> &selected) {
  Input input;
  input.total_tracks = total_tracks;
  input.selected_tracks = static_cast<int>(selected.size());
  input.total_length_ns = total_length_ns;
  input.selected_length_ns = SelectedLengthNs(songs, selected);
  return input;
}

inline int64_t DurationNs(const Input &input) { return input.selected_tracks > 1 ? input.selected_length_ns : input.total_length_ns; }

inline std::string TrackCountText(int tracks) {
  if (tracks == 1) {
    return "1 track";
  }
  return std::to_string(tracks) + " tracks";
}

inline std::string Format(const Input &input) {
  std::string summary;
  if (input.selected_tracks > 1) {
    summary += std::to_string(input.selected_tracks) + " selected of ";
  }
  summary += TrackCountText(input.total_tracks);
  const int64_t nanoseconds = DurationNs(input);
  if (nanoseconds > 0) {
    summary += " - [ " + Utilities::WordyTimeNanosec(static_cast<uint64_t>(nanoseconds)) + " ]";
  }
  return summary;
}

}  // namespace PlaylistSummary

#endif  // STRAWBERRY_PLAYLISTSUMMARY_H
