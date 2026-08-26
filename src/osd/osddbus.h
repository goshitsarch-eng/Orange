#ifndef STRAWBERRY_OSDDBUS_H
#define STRAWBERRY_OSDDBUS_H

#include <string>

class OSDDbus {
 public:
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");
};

#endif
