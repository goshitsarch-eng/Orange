#ifndef STRAWBERRY_CONTEXTTECHNICAL_H
#define STRAWBERRY_CONTEXTTECHNICAL_H

#include "core/song.h"

#include <string>
#include <utility>
#include <vector>

namespace ContextTechnical {

std::vector<std::pair<std::string, std::string>> Rows(const Song &song);
std::string Headline(const Song &song, const std::string &format);
std::string Summary(const Song &song, const std::string &format);

}  // namespace ContextTechnical

#endif
