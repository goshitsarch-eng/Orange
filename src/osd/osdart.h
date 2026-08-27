#ifndef STRAWBERRY_OSDART_H
#define STRAWBERRY_OSDART_H

#include <string>
#include <vector>

namespace OSDArt {

inline bool ShouldAttachArt(bool show_art, const std::vector<unsigned char> &art) { return show_art && !art.empty(); }

inline const std::vector<unsigned char> &EffectiveArt(bool show_art, const std::vector<unsigned char> &art) {
  static const std::vector<unsigned char> empty;
  return ShouldAttachArt(show_art, art) ? art : empty;
}

}  // namespace OSDArt

#endif
