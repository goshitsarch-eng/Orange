#include "playlist/playlistdelegates.h"

#include "core/settings.h"
#include "utilities/fileutils.h"
#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "widgets/ratingpainter.h"

#include <cstdlib>

std::string PlaylistDelegates::ColumnTitle(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Track:
      return "Track";
    case PlaylistColumn::Title:
      return "Title";
    case PlaylistColumn::TitleSort:
      return "Title sort";
    case PlaylistColumn::Artist:
      return "Artist";
    case PlaylistColumn::ArtistSort:
      return "Artist sort";
    case PlaylistColumn::Album:
      return "Album";
    case PlaylistColumn::AlbumSort:
      return "Album sort";
    case PlaylistColumn::AlbumArtist:
      return "Album artist";
    case PlaylistColumn::AlbumArtistSort:
      return "Album artist sort";
    case PlaylistColumn::Performer:
      return "Performer";
    case PlaylistColumn::PerformerSort:
      return "Performer sort";
    case PlaylistColumn::Composer:
      return "Composer";
    case PlaylistColumn::ComposerSort:
      return "Composer sort";
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
    case PlaylistColumn::TitleSort:
    case PlaylistColumn::URL:
    case PlaylistColumn::Filename:
    case PlaylistColumn::Comment:
      return 200;
    case PlaylistColumn::Artist:
    case PlaylistColumn::ArtistSort:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumSort:
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::AlbumArtistSort:
    case PlaylistColumn::Performer:
    case PlaylistColumn::PerformerSort:
    case PlaylistColumn::Composer:
    case PlaylistColumn::ComposerSort:
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
    case PlaylistColumn::TitleSort:
      return song.titlesort();
    case PlaylistColumn::Artist:
      return song.artist();
    case PlaylistColumn::ArtistSort:
      return song.artistsort();
    case PlaylistColumn::Album:
      return song.album();
    case PlaylistColumn::AlbumSort:
      return song.albumsort();
    case PlaylistColumn::AlbumArtist:
      return song.EffectiveAlbumartist();
    case PlaylistColumn::AlbumArtistSort:
      return song.albumartistsort();
    case PlaylistColumn::Performer:
      return song.performer();
    case PlaylistColumn::PerformerSort:
      return song.performersort();
    case PlaylistColumn::Composer:
      return song.composer();
    case PlaylistColumn::ComposerSort:
      return song.composersort();
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

bool PlaylistDelegates::ColumnIsEditable(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::Title:
    case PlaylistColumn::TitleSort:
    case PlaylistColumn::Artist:
    case PlaylistColumn::ArtistSort:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumSort:
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::AlbumArtistSort:
    case PlaylistColumn::Composer:
    case PlaylistColumn::ComposerSort:
    case PlaylistColumn::Performer:
    case PlaylistColumn::PerformerSort:
    case PlaylistColumn::Grouping:
    case PlaylistColumn::Track:
    case PlaylistColumn::Disc:
    case PlaylistColumn::Year:
    case PlaylistColumn::Genre:
    case PlaylistColumn::Comment:
      return true;
    default:
      return false;
  }
}

bool PlaylistDelegates::SetColumnValue(Song &song, PlaylistColumn column, const std::string &value) {
  if (!ColumnIsEditable(column)) {
    return false;
  }
  auto parse_int = [](const std::string &text) {
    if (text.empty()) {
      return -1;
    }
    char *end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (!end || end == text.c_str()) {
      return -1;
    }
    return static_cast<int>(parsed);
  };
  switch (column) {
    case PlaylistColumn::Title:
      song.set_title(value);
      return true;
    case PlaylistColumn::TitleSort:
      song.set_titlesort(value);
      return true;
    case PlaylistColumn::Artist:
      song.set_artist(value);
      return true;
    case PlaylistColumn::ArtistSort:
      song.set_artistsort(value);
      return true;
    case PlaylistColumn::Album:
      song.set_album(value);
      return true;
    case PlaylistColumn::AlbumSort:
      song.set_albumsort(value);
      return true;
    case PlaylistColumn::AlbumArtist:
      song.set_albumartist(value);
      return true;
    case PlaylistColumn::AlbumArtistSort:
      song.set_albumartistsort(value);
      return true;
    case PlaylistColumn::Composer:
      song.set_composer(value);
      return true;
    case PlaylistColumn::ComposerSort:
      song.set_composersort(value);
      return true;
    case PlaylistColumn::Performer:
      song.set_performer(value);
      return true;
    case PlaylistColumn::PerformerSort:
      song.set_performersort(value);
      return true;
    case PlaylistColumn::Grouping:
      song.set_grouping(value);
      return true;
    case PlaylistColumn::Track:
      song.set_track(parse_int(value));
      return true;
    case PlaylistColumn::Disc:
      song.set_disc(parse_int(value));
      return true;
    case PlaylistColumn::Year:
      song.set_year(parse_int(value));
      return true;
    case PlaylistColumn::Genre:
      song.set_genre(value);
      return true;
    case PlaylistColumn::Comment:
      song.set_comment(value);
      return true;
    default:
      return false;
  }
}
