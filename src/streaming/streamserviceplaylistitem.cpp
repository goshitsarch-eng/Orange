#include "streaming/streamserviceplaylistitem.h"

StreamServicePlaylistItem::StreamServicePlaylistItem(const Song &song) : song_(song) {}

std::string StreamServicePlaylistItem::DisplayText() const {
  const std::string text = song_.PrettyTitleWithArtist();
  return text.empty() ? song_.url() : text;
}
