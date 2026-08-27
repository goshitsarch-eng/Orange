#include "smartplaylists/playlistgenerator.h"

#include "smartplaylists/playlistquerygenerator.h"

SongList PlaylistGenerator::GenerateMore(int) { return {}; }

std::shared_ptr<PlaylistGenerator> PlaylistGenerator::Create(Type type) {
  if (type == Type::Query) {
    return std::make_shared<PlaylistQueryGenerator>();
  }
  return {};
}
