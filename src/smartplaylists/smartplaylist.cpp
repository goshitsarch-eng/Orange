#include "smartplaylists/smartplaylist.h"

#include "collection/collectionbackend.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdlib>

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

bool SmartPlaylistTerm::Matches(const Song &song) const {
  const std::string text = FieldText(song, field);
  const double number = FieldNumber(song, field);
  const double wanted = std::strtod(value.c_str(), nullptr);
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
  }
  return false;
}

SongList SmartPlaylistSearch::Search(const SongList &songs) const {
  SongList result;
  for (const Song &song : songs) {
    bool matches = type == SearchType::And;
    if (terms.empty()) {
      matches = true;
    } else {
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
          "Not empty"};
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
