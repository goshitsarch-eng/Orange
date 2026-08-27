#include "collection/collectionitemdelegate.h"

#include <algorithm>

namespace CollectionItemDelegate {

std::string PrimaryText(const CollectionItem *item) {
  if (!item) {
    return {};
  }
  if (item->type == CollectionItem::Type::Song) {
    return item->metadata.PrettyTitle().empty() ? item->metadata.url() : item->metadata.PrettyTitle();
  }
  if (item->type == CollectionItem::Type::LoadingIndicator) {
    return "Loading…";
  }
  return item->display_text;
}

std::string SecondaryText(const CollectionItem *item) {
  if (!item || item->type != CollectionItem::Type::Song) {
    return {};
  }
  std::string text = item->metadata.artist();
  if (!item->metadata.album().empty()) {
    if (!text.empty()) {
      text += " · ";
    }
    text += item->metadata.album();
  }
  return text;
}

int Indent(const CollectionItem *item) { return item ? std::max(0, item->container_level + (item->type == CollectionItem::Type::Song ? 1 : 0)) : 0; }

}  // namespace CollectionItemDelegate
