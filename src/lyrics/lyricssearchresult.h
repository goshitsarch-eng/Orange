#ifndef STRAWBERRY_LYRICSSEARCHRESULT_H
#define STRAWBERRY_LYRICSSEARCHRESULT_H

#include <string>
#include <vector>

class LyricsSearchResult {
 public:
  std::string provider;
  std::string lyrics;
  float score = 0.0f;
};

using LyricsSearchResults = std::vector<LyricsSearchResult>;

#endif
