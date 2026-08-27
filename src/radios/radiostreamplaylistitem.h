#ifndef STRAWBERRY_RADIOSTREAMPLAYLISTITEM_H
#define STRAWBERRY_RADIOSTREAMPLAYLISTITEM_H

#include "playlist/streamplaylistitem.h"
#include "radios/radiochannel.h"

class RadioStreamPlaylistItem : public StreamPlaylistItem {
 public:
  explicit RadioStreamPlaylistItem(const RadioChannel &channel);
};

#endif
