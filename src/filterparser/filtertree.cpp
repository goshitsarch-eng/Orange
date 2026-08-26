#include "filterparser/filtertree.h"

#include "utilities/fileutils.h"

std::string FilterTree::DataFromColumn(FilterColumn column, const Song &song) {
  switch (column) {
    case FilterColumn::Title:
      return song.title();
    case FilterColumn::TitleSort:
      return song.titlesort();
    case FilterColumn::Album:
      return song.album();
    case FilterColumn::AlbumSort:
      return song.albumsort();
    case FilterColumn::Artist:
      return song.artist();
    case FilterColumn::ArtistSort:
      return song.artistsort();
    case FilterColumn::AlbumArtist:
      return song.albumartist();
    case FilterColumn::AlbumArtistSort:
      return song.albumartistsort();
    case FilterColumn::Composer:
      return song.composer();
    case FilterColumn::ComposerSort:
      return song.composersort();
    case FilterColumn::Performer:
      return song.performer();
    case FilterColumn::PerformerSort:
      return song.performersort();
    case FilterColumn::Grouping:
      return song.grouping();
    case FilterColumn::Genre:
      return song.genre();
    case FilterColumn::Comment:
      return song.comment();
    case FilterColumn::Filename:
      return song.basefilename().empty() ? FileUtils::BaseName(FileUtils::PathFromUri(song.url())) : song.basefilename();
    case FilterColumn::URL:
      return song.url();
    default:
      return {};
  }
}

double FilterTree::NumericFromColumn(FilterColumn column, const Song &song) {
  switch (column) {
    case FilterColumn::Track:
      return song.track();
    case FilterColumn::Year:
      return song.year();
    case FilterColumn::Samplerate:
      return song.samplerate();
    case FilterColumn::Bitdepth:
      return song.bitdepth();
    case FilterColumn::Bitrate:
      return song.bitrate();
    case FilterColumn::Playcount:
      return song.playcount();
    case FilterColumn::Skipcount:
      return song.skipcount();
    case FilterColumn::Length:
      return static_cast<double>(song.length_nanosec());
    case FilterColumn::Rating:
      return song.rating();
    case FilterColumn::Age:
    case FilterColumn::Added:
      return static_cast<double>(song.ctime());
    case FilterColumn::LastPlayed:
      return static_cast<double>(song.lastplayed());
    default:
      return 0;
  }
}

bool FilterTree::IsNumeric(FilterColumn column) {
  switch (column) {
    case FilterColumn::Track:
    case FilterColumn::Year:
    case FilterColumn::Samplerate:
    case FilterColumn::Bitdepth:
    case FilterColumn::Bitrate:
    case FilterColumn::Playcount:
    case FilterColumn::Skipcount:
    case FilterColumn::Length:
    case FilterColumn::Rating:
    case FilterColumn::Age:
    case FilterColumn::Added:
    case FilterColumn::LastPlayed:
      return true;
    default:
      return false;
  }
}
