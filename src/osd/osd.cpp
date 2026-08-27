#include "osd/osd.h"

#include "osd/osdpretty.h"

void OSD::ShowPretty(const std::string &summary, const std::string &body, const std::vector<unsigned char> &art) {
  if (pretty_) {
    pretty_->ShowMessage(summary, body, art);
  }
}
