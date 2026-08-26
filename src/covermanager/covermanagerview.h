#ifndef STRAWBERRY_COVERMANAGERVIEW_H
#define STRAWBERRY_COVERMANAGERVIEW_H

#include "covermanager/albumcovermanagerlist.h"

namespace CoverManagerView {

inline const char *ButtonLabel() { return "View"; }
inline const char *ButtonIcon() { return "view-grid-symbolic"; }
inline const char *SearchPlaceholder() { return "Enter search terms here"; }

inline constexpr const char *kLabels[] = {"All albums", "Albums with covers", "Albums without covers"};
inline constexpr int kCount = 3;

inline int ClampIndex(int index) {
  if (index < 0) {
    return 0;
  }
  if (index >= kCount) {
    return kCount - 1;
  }
  return index;
}

inline AlbumCoverManagerList::HideCovers HideFromIndex(int index) {
  switch (ClampIndex(index)) {
    case 1:
      return AlbumCoverManagerList::HideCovers::WithoutCovers;
    case 2:
      return AlbumCoverManagerList::HideCovers::WithCovers;
    default:
      return AlbumCoverManagerList::HideCovers::None;
  }
}

inline int IndexFromHide(AlbumCoverManagerList::HideCovers hide) {
  switch (hide) {
    case AlbumCoverManagerList::HideCovers::WithoutCovers:
      return 1;
    case AlbumCoverManagerList::HideCovers::WithCovers:
      return 2;
    case AlbumCoverManagerList::HideCovers::None:
    default:
      return 0;
  }
}

}  // namespace CoverManagerView

#endif  // STRAWBERRY_COVERMANAGERVIEW_H
