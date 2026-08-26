#ifndef STRAWBERRY_STREAMINGMEDIAID_H
#define STRAWBERRY_STREAMINGMEDIAID_H

#include <string>

inline std::string StreamingMediaId(const std::string &url) {
  std::string id = url;
  const auto scheme = id.find("://");
  if (scheme != std::string::npos) {
    id = id.substr(scheme + 3);
  }
  if (!id.empty() && id.front() == '/') {
    id.erase(id.begin());
  }
  const auto slash = id.rfind('/');
  if (slash != std::string::npos) {
    id = id.substr(slash + 1);
  }
  return id;
}

#endif
