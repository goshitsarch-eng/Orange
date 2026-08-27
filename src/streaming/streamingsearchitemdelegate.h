#ifndef STRAWBERRY_STREAMINGSEARCHITEMDELEGATE_H
#define STRAWBERRY_STREAMINGSEARCHITEMDELEGATE_H

#include "core/song.h"

#include <string>

namespace StreamingSearchItemDelegate {

std::string PrimaryText(const Song &song);
std::string SecondaryText(const Song &song);

}  // namespace StreamingSearchItemDelegate

#endif
