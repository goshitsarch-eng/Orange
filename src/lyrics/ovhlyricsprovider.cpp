#include "lyrics/ovhlyricsprovider.h"

OVHLyricsProvider::OVHLyricsProvider() : JsonLyricsProvider("Lyrics.ovh", "https://api.lyrics.ovh/v1/{artist}/{title}") {}
