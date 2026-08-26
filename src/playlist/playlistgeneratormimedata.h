#ifndef STRAWBERRY_PLAYLISTGENERATORMIMEDATA_H
#define STRAWBERRY_PLAYLISTGENERATORMIMEDATA_H

#include "core/mimedata.h"

#include <string>

struct PlaylistGeneratorMimeData : MimeData {
  std::string generator_name;
  bool dynamic = false;
};

#endif
