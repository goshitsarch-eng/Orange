#include "playlist/playlistitemsavedata.h"

PlaylistItemSaveData::PlaylistItemSaveData(const Song &song, const std::string &uuid)
    : source(song.source()), uuid(uuid), collection_id(song.id()), song(song) {}
