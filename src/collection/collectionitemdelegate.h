#ifndef STRAWBERRY_COLLECTIONITEMDELEGATE_H
#define STRAWBERRY_COLLECTIONITEMDELEGATE_H

#include "collection/collectionitem.h"

#include <string>

namespace CollectionItemDelegate {

std::string PrimaryText(const CollectionItem *item);
std::string SecondaryText(const CollectionItem *item);
int Indent(const CollectionItem *item);
inline bool IsDivider(const CollectionItem *item) { return item && item->type == CollectionItem::Type::Divider; }
inline bool ShouldShowTooltip(const std::string &text) { return !text.empty(); }
inline const std::string &TooltipText(const std::string &text) { return text; }

}  // namespace CollectionItemDelegate

#endif
