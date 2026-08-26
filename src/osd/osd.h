#ifndef STRAWBERRY_OSD_H
#define STRAWBERRY_OSD_H

#include "osd/osdbase.h"

class OSD : public OSDBase {
 public:
  using OSDBase::OSDBase;
  void ShowPretty(const std::string &summary, const std::string &body, const std::vector<unsigned char> &art = {});
};

#endif
