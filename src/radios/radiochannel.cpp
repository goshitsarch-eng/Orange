#include "radios/radiochannel.h"

Song RadioChannel::ToSong() const {
  Song song(source);
  song.set_title(name.empty() ? url : name);
  song.set_url(url);
  song.set_art_automatic(thumbnail_url);
  song.set_genre(tags);
  song.set_valid(true);
  return song;
}
