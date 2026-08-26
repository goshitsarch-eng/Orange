#ifndef STRAWBERRY_PLAYLISTDELEGATES_H
#define STRAWBERRY_PLAYLISTDELEGATES_H

#include "core/song.h"

#include <string>

enum class PlaylistColumn {
  Track,
  Title,
  Artist,
  Album,
  AlbumArtist,
  Performer,
  Composer,
  Year,
  OriginalYear,
  Disc,
  Length,
  Genre,
  Samplerate,
  Bitdepth,
  Bitrate,
  URL,
  Filename,
  Filesize,
  Filetype,
  DateCreated,
  DateModified,
  PlayCount,
  SkipCount,
  LastPlayed,
  Comment,
  Grouping,
  Source,
  Moodbar,
  Rating,
  HasCUE,
  EBUR128I,
  EBUR128LRA,
  BPM,
  Mood,
  InitialKey,
  Count
};

namespace PlaylistDelegates {

std::string ColumnTitle(PlaylistColumn column);
std::string ColumnText(const Song &song, PlaylistColumn column);
int ColumnWidth(PlaylistColumn column);
bool ColumnVisible(PlaylistColumn column);
std::string RatingStars(float rating);

}  // namespace PlaylistDelegates

#endif
