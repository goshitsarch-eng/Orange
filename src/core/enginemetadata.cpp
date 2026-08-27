#include "core/enginemetadata.h"

Song EngineMetadata::ToSong(Song::Source source) const {
  Song song(source);
  song.set_url(media_url);
  if (!stream_url.empty()) {
    song.set_stream_url(stream_url);
  }
  song.set_title(title);
  song.set_artist(artist);
  song.set_album(album);
  song.set_comment(comment);
  song.set_genre(genre);
  song.set_lyrics(lyrics);
  song.set_year(year);
  song.set_track(track);
  song.set_length_nanosec(length_nanosec);
  song.set_filetype(filetype);
  song.set_bitrate(bitrate);
  song.set_samplerate(samplerate);
  song.set_bitdepth(bitdepth);
  song.set_valid(true);
  return song;
}
