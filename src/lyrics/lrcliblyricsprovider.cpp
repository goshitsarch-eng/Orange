#include "lyrics/lrcliblyricsprovider.h"

LrcLibLyricsProvider::LrcLibLyricsProvider()
    : JsonLyricsProvider("LrcLib", "https://lrclib.net/api/get?artist_name={artist}&track_name={title}&album_name={album}") {}
