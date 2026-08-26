#ifndef STRAWBERRY_EDITTAGFIELDS_H
#define STRAWBERRY_EDITTAGFIELDS_H

#include "core/song.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace EditTagFields {

std::pair<std::string, bool> CommonValue(const SongList &songs, const std::function<std::string(const Song &)> &getter);
void ApplyField(Song *song, const std::string &name, const std::string &value);
void ApplyChangedFields(SongList *songs, const std::vector<std::pair<std::string, std::string>> &changed);
void ResetPlayStatistics(Song *song);
void ResetPlayStatistics(SongList *songs);
int WrapIndex(int current, int delta, int count);
std::string SongRowLabel(const Song &song);

}  // namespace EditTagFields

#endif
