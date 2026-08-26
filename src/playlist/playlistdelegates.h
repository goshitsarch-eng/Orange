#ifndef STRAWBERRY_PLAYLISTDELEGATES_H
#define STRAWBERRY_PLAYLISTDELEGATES_H

#include "core/song.h"

#include <string>

enum class PlaylistSortOrder { Toggle, Ascending, Descending, Clear };

enum class PlaylistColumnAlign { Left, Center, Right };

enum class PlaylistColumn {
  Track,
  Title,
  TitleSort,
  Artist,
  ArtistSort,
  Album,
  AlbumSort,
  AlbumArtist,
  AlbumArtistSort,
  Performer,
  PerformerSort,
  Composer,
  ComposerSort,
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
  Queue,
  Count
};

namespace PlaylistDelegates {

std::string ColumnTitle(PlaylistColumn column);
std::string ColumnText(const Song &song, PlaylistColumn column);
int ColumnWidth(PlaylistColumn column);
bool ColumnVisible(PlaylistColumn column);
bool ColumnIsEditable(PlaylistColumn column);
bool SetColumnValue(Song &song, PlaylistColumn column, const std::string &value);
std::string RatingStars(float rating);

}  // namespace PlaylistDelegates

#endif
