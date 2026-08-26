#ifndef STRAWBERRY_JSONUTILS_H
#define STRAWBERRY_JSONUTILS_H

#include "core/song.h"

#include <string>
#include <vector>

namespace JsonUtils {

bool LooksLikeImage(const std::string &data);
std::string StripHtml(const std::string &html);
std::string GetString(const std::string &json, const std::vector<std::string> &path);
std::string FindStringByKeys(const std::string &json, const std::vector<std::string> &keys);
std::string FindFirstImageUrl(const std::string &json);
std::string FindCoverUrl(const std::string &json);
std::string ExtractLyrics(const std::string &body);
SongList ParseSongs(const std::string &json, Song::Source source = Song::Source::Stream);

}  // namespace JsonUtils

#endif
