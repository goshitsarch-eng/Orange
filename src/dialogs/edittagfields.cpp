#include "dialogs/edittagfields.h"

#include <cstdlib>

namespace EditTagFields {

std::pair<std::string, bool> CommonValue(const SongList &songs, const std::function<std::string(const Song &)> &getter) {
  if (songs.empty()) {
    return {{}, false};
  }
  std::string value = getter(songs.front());
  for (const Song &song : songs) {
    if (getter(song) != value) {
      return {{}, true};
    }
  }
  return {value, false};
}

void ApplyField(Song *song, const std::string &name, const std::string &value) {
  if (!song) {
    return;
  }
  const auto as_int = [&]() -> int { return value.empty() ? -1 : std::atoi(value.c_str()); };
  if (name == "Title") song->set_title(value);
  else if (name == "Artist") song->set_artist(value);
  else if (name == "Album") song->set_album(value);
  else if (name == "Album artist") song->set_albumartist(value);
  else if (name == "Composer") song->set_composer(value);
  else if (name == "Performer") song->set_performer(value);
  else if (name == "Grouping") song->set_grouping(value);
  else if (name == "Comment") song->set_comment(value);
  else if (name == "Genre") song->set_genre(value);
  else if (name == "Lyrics") song->set_lyrics(value);
  else if (name == "Mood") song->set_mood(value);
  else if (name == "Initial key") song->set_initial_key(value);
  else if (name == "Title sort") song->set_titlesort(value);
  else if (name == "Artist sort") song->set_artistsort(value);
  else if (name == "Album sort") song->set_albumsort(value);
  else if (name == "Album artist sort") song->set_albumartistsort(value);
  else if (name == "Year") song->set_year(as_int());
  else if (name == "Original year") song->set_originalyear(as_int());
  else if (name == "Track") song->set_track(as_int());
  else if (name == "Disc") song->set_disc(as_int());
  else if (name == "BPM") song->set_bpm(value.empty() ? -1.0f : std::strtof(value.c_str(), nullptr));
}

void ApplyChangedFields(SongList *songs, const std::vector<std::pair<std::string, std::string>> &changed) {
  if (!songs) {
    return;
  }
  for (Song &song : *songs) {
    for (const auto &field : changed) {
      ApplyField(&song, field.first, field.second);
    }
  }
}

void ResetPlayStatistics(Song *song) {
  if (!song) {
    return;
  }
  song->set_playcount(0);
  song->set_skipcount(0);
  song->set_lastplayed(-1);
}

void ResetPlayStatistics(SongList *songs) {
  if (!songs) {
    return;
  }
  for (Song &song : *songs) {
    ResetPlayStatistics(&song);
  }
}

int WrapIndex(int current, int delta, int count) {
  if (count <= 0) {
    return 0;
  }
  int index = current + delta;
  while (index < 0) {
    index += count;
  }
  return index % count;
}

std::string SongRowLabel(const Song &song) {
  const std::string label = song.PrettyTitleWithArtist();
  return label.empty() ? song.url() : label;
}

}  // namespace EditTagFields
