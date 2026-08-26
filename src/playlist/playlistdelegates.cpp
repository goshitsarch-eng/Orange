#include "playlist/playlistdelegates.h"

#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "widgets/ratingpainter.h"

std::string PlaylistDelegates::ColumnTitle(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
      return "Track";
    case PlaylistColumn::Title:
      return "Title";
    case PlaylistColumn::Artist:
      return "Artist";
    case PlaylistColumn::Album:
      return "Album";
    case PlaylistColumn::AlbumArtist:
      return "Album artist";
    case PlaylistColumn::Performer:
      return "Performer";
    case PlaylistColumn::Composer:
      return "Composer";
    case PlaylistColumn::Year:
      return "Year";
    case PlaylistColumn::OriginalYear:
      return "Original year";
    case PlaylistColumn::Disc:
      return "Disc";
    case PlaylistColumn::Length:
      return "Length";
    case PlaylistColumn::Genre:
      return "Genre";
    case PlaylistColumn::Samplerate:
      return "Sample rate";
    case PlaylistColumn::Bitdepth:
      return "Bit depth";
    case PlaylistColumn::Bitrate:
      return "Bitrate";
    case PlaylistColumn::URL:
      return "URL";
    case PlaylistColumn::Filename:
      return "Filename";
    case PlaylistColumn::Filesize:
      return "Filesize";
    case PlaylistColumn::Filetype:
      return "Filetype";
    case PlaylistColumn::DateCreated:
      return "Date created";
    case PlaylistColumn::DateModified:
      return "Date modified";
    case PlaylistColumn::PlayCount:
      return "Plays";
    case PlaylistColumn::SkipCount:
      return "Skips";
    case PlaylistColumn::LastPlayed:
      return "Last played";
    case PlaylistColumn::Comment:
      return "Comment";
    case PlaylistColumn::Grouping:
      return "Grouping";
    case PlaylistColumn::Source:
      return "Source";
    case PlaylistColumn::Moodbar:
      return "Moodbar";
    case PlaylistColumn::Rating:
      return "Rating";
    case PlaylistColumn::HasCUE:
      return "CUE";
    case PlaylistColumn::EBUR128I:
      return "EBU R128 I";
    case PlaylistColumn::EBUR128LRA:
      return "EBU R128 LRA";
    case PlaylistColumn::BPM:
      return "BPM";
    case PlaylistColumn::Mood:
      return "Mood";
    case PlaylistColumn::InitialKey:
      return "Initial key";
    case PlaylistColumn::Count:
      break;
  }
  return {};
}

int PlaylistDelegates::ColumnWidth(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Title:
    case PlaylistColumn::URL:
    case PlaylistColumn::Filename:
    case PlaylistColumn::Comment:
      return 200;
    case PlaylistColumn::Artist:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::Performer:
    case PlaylistColumn::Composer:
      return 150;
    default:
      return 80;
  }
}

bool PlaylistDelegates::ColumnVisible(PlaylistColumn column) {
  Settings settings;
  settings.BeginGroup("Playlist");
  const std::string enabled =
      settings.Value("columns", "Track,Title,Artist,Album,Album artist,Length,Year,Genre,Bitrate,Sample rate,Plays,Rating,Filename");
  const std::string title = ColumnTitle(column);
  for (const std::string &part : StrUtils::Split(enabled, ',')) {
    if (part == title) {
      return true;
    }
  }
  return false;
}

std::string PlaylistDelegates::RatingStars(float rating) { return RatingPainter::Stars(rating); }

std::string PlaylistDelegates::ColumnText(const Song &song, PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
      return song.track() > 0 ? std::to_string(song.track()) : "";
    case PlaylistColumn::Title:
      return song.PrettyTitle();
    case PlaylistColumn::Artist:
      return song.artist();
    case PlaylistColumn::Album:
      return song.album();
    case PlaylistColumn::AlbumArtist:
      return song.EffectiveAlbumartist();
    case PlaylistColumn::Performer:
      return song.performer();
    case PlaylistColumn::Composer:
      return song.composer();
    case PlaylistColumn::Year:
      return song.year() > 0 ? std::to_string(song.year()) : "";
    case PlaylistColumn::OriginalYear:
      return song.originalyear() > 0 ? std::to_string(song.originalyear()) : "";
    case PlaylistColumn::Disc:
      return song.disc() > 0 ? std::to_string(song.disc()) : "";
    case PlaylistColumn::Length:
      return Utilities::PrettyTimeNanosec(song.length_nanosec());
    case PlaylistColumn::Genre:
      return song.genre();
    case PlaylistColumn::Bitrate:
      return song.bitrate() > 0 ? std::to_string(song.bitrate()) : "";
    case PlaylistColumn::Samplerate:
      return song.samplerate() > 0 ? std::to_string(song.samplerate()) : "";
    case PlaylistColumn::Bitdepth:
      return song.bitdepth() > 0 ? std::to_string(song.bitdepth()) : "";
    case PlaylistColumn::URL:
      return song.url();
    case PlaylistColumn::Filename:
      return song.basefilename().empty() ? FileUtils::BaseName(FileUtils::PathFromUri(song.url())) : song.basefilename();
    case PlaylistColumn::Filesize:
      return song.filesize() > 0 ? std::to_string(song.filesize()) : "";
    case PlaylistColumn::Filetype:
      return Song::FiletypeToString(song.filetype());
    case PlaylistColumn::DateCreated:
      return song.ctime() > 0 ? std::to_string(song.ctime()) : "";
    case PlaylistColumn::DateModified:
      return song.mtime() > 0 ? std::to_string(song.mtime()) : "";
    case PlaylistColumn::PlayCount:
      return std::to_string(song.playcount());
    case PlaylistColumn::SkipCount:
      return std::to_string(song.skipcount());
    case PlaylistColumn::LastPlayed:
      return song.lastplayed() > 0 ? std::to_string(song.lastplayed()) : "";
    case PlaylistColumn::Comment:
      return song.comment();
    case PlaylistColumn::Grouping:
      return song.grouping();
    case PlaylistColumn::Source:
      return Song::SourceToString(song.source());
    case PlaylistColumn::Moodbar:
      return song.mood().empty() ? "" : "●";
    case PlaylistColumn::Rating:
      return RatingStars(song.rating());
    case PlaylistColumn::HasCUE:
      return song.cue_path().empty() ? "" : "CUE";
    case PlaylistColumn::EBUR128I:
      return song.ebur128_integrated_loudness_lufs() ? std::to_string(*song.ebur128_integrated_loudness_lufs()) : "";
    case PlaylistColumn::EBUR128LRA:
      return song.ebur128_loudness_range_lu() ? std::to_string(*song.ebur128_loudness_range_lu()) : "";
    case PlaylistColumn::BPM:
      return song.bpm() > 0 ? std::to_string(song.bpm()) : "";
    case PlaylistColumn::Mood:
      return song.mood();
    case PlaylistColumn::InitialKey:
      return song.initial_key();
    case PlaylistColumn::Count:
      break;
  }
  return {};
}
