#ifndef STRAWBERRY_ALBUMCOVERMANAGERSELECTION_H
#define STRAWBERRY_ALBUMCOVERMANAGERSELECTION_H

#include <string>

namespace AlbumCoverManagerSelection {

inline bool PreferSelection(size_t selected) { return selected > 0; }

inline std::string StatusText(size_t albums, size_t with_cover, size_t selected) {
  const size_t missing = albums > with_cover ? albums - with_cover : 0;
  std::string text = std::to_string(albums) + " albums · " + std::to_string(with_cover) + " with artwork · " + std::to_string(missing) + " missing";
  if (selected > 0) {
    text += " · " + std::to_string(selected) + " selected";
  }
  return text;
}

}  // namespace AlbumCoverManagerSelection

#endif
