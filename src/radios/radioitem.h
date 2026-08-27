#ifndef STRAWBERRY_RADIOITEM_H
#define STRAWBERRY_RADIOITEM_H

#include "radios/radiochannel.h"

struct RadioItem {
  RadioChannel channel;
  bool expanded = false;
};

#endif
