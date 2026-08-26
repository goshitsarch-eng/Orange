#include "radios/radiostreamplaylistitem.h"

RadioStreamPlaylistItem::RadioStreamPlaylistItem(const RadioChannel &channel) : StreamPlaylistItem(channel.ToSong()) {}
