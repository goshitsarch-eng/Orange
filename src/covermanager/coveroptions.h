#ifndef STRAWBERRY_COVEROPTIONS_H
#define STRAWBERRY_COVEROPTIONS_H

#include <string>

struct CoverOptions {
  int desired_height = 300;
  bool pad = false;
  bool scale = true;
  std::string default_cover;
};

#endif
