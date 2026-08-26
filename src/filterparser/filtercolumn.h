#ifndef STRAWBERRY_FILTERCOLUMN_H
#define STRAWBERRY_FILTERCOLUMN_H

enum class FilterColumn {
  Unknown,
  Title,
  TitleSort,
  Album,
  AlbumSort,
  Artist,
  ArtistSort,
  AlbumArtist,
  AlbumArtistSort,
  Composer,
  ComposerSort,
  Performer,
  PerformerSort,
  Grouping,
  Genre,
  Comment,
  Filename,
  URL,
  Track,
  Year,
  Samplerate,
  Bitdepth,
  Bitrate,
  Playcount,
  Skipcount,
  Length,
  Rating,
  Age,
  Added,
  LastPlayed,
};

enum class FilterOperator {
  None,
  Eq,
  Ne,
  Gt,
  Ge,
  Lt,
  Le,
};

#endif
