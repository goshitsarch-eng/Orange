#include "radios/radiomimedata.h"

std::string RadioMimeData::format() const {
  std::string out;
  for (const RadioChannel &channel : channels) {
    if (!out.empty()) {
      out += "\n";
    }
    out += channel.name + "\t" + channel.url;
  }
  return out;
}
