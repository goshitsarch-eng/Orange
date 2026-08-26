#include "core/mimedata.h"

std::string MimeData::text() const {
  if (!urls.empty()) {
    return urls;
  }
  std::string out;
  for (const Song &song : songs) {
    if (!out.empty()) {
      out += "\n";
    }
    out += song.url();
  }
  return out;
}
