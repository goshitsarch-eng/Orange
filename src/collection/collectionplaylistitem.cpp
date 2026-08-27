#include "collection/collectionplaylistitem.h"

CollectionPlaylistItem::CollectionPlaylistItem(const Song &song) : song_(song) {}

std::string CollectionPlaylistItem::DisplayText() const {
  const std::string text = song_.PrettyTitleWithArtist();
  return text.empty() ? song_.url() : text;
}
