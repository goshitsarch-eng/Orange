#ifndef STRAWBERRY_CONTEXTLYRICS_H
#define STRAWBERRY_CONTEXTLYRICS_H

#include <string>

namespace ContextLyrics {

inline std::string Attribution(const std::string &provider) {
  if (provider.empty()) {
    return {};
  }
  return "Source: " + provider;
}

}  // namespace ContextLyrics

#endif
