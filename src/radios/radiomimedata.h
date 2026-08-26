#ifndef STRAWBERRY_RADIOMIMEDATA_H
#define STRAWBERRY_RADIOMIMEDATA_H

#include "radios/radiochannel.h"

#include <string>
#include <vector>

struct RadioMimeData {
  std::vector<RadioChannel> channels;
  std::string format() const;
};

#endif
