#ifndef STRAWBERRY_COLLECTIONSEARCHSYNC_H
#define STRAWBERRY_COLLECTIONSEARCHSYNC_H

#include <string>

namespace CollectionSearchSync {

// Qt CollectionFilterWidget::ShowInCollection sets the search field whenever Search for this / Show in collection apply a query.
inline bool ShouldUpdateEntry(bool update_text) { return update_text; }

inline bool TextDiffers(const char *current, const std::string &next) {
  const char *text = current ? current : "";
  return next != text;
}

}  // namespace CollectionSearchSync

#endif
