#ifndef STRAWBERRY_COLLECTIONITEMDELEGATE_H
#define STRAWBERRY_COLLECTIONITEMDELEGATE_H

#include "collection/collectionitem.h"

#include <string>

namespace CollectionItemDelegate {

std::string PrimaryText(const CollectionItem *item);
std::string SecondaryText(const CollectionItem *item);
int Indent(const CollectionItem *item);

}  // namespace CollectionItemDelegate

#endif
