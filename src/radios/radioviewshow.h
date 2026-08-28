#ifndef STRAWBERRY_RADIOVIEWSHOW_H
#define STRAWBERRY_RADIOVIEWSHOW_H

namespace RadioViewShow {

// Qt RadioView::showEvent emits GetChannels once, the first time the tree is shown.
inline bool ShouldFetchOnShow(bool initialized) { return !initialized; }

}  // namespace RadioViewShow

#endif
