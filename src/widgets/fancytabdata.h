#ifndef STRAWBERRY_FANCYTABDATA_H
#define STRAWBERRY_FANCYTABDATA_H

#include <string>

struct FancyTabData {
  std::string id;
  std::string title;
  std::string icon;
  bool enabled = true;
};

#endif
