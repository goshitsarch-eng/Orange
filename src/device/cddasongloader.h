#ifndef STRAWBERRY_CDDASONGLOADER_H
#define STRAWBERRY_CDDASONGLOADER_H

#include "core/song.h"

#include <string>
#include <vector>

class CddaSongLoader {
 public:
  static SongList Songs(int first_track, int last_track, const std::vector<int64_t> &lengths_nanosec,
                       const std::string &device_path = {});
  static SongList LoadDevice(const std::string &device_path);
};

#endif
