#ifndef STRAWBERRY_URLHANDLERS_H
#define STRAWBERRY_URLHANDLERS_H

#include "core/urlhandler.h"

#include <map>
#include <string>
#include <vector>

class UrlHandlers {
 public:
  void AddHandler(UrlHandler *handler);
  UrlHandler *HandlerForUrl(const std::string &url) const;
  std::vector<std::string> Schemes() const;

 private:
  std::map<std::string, UrlHandler *> handlers_;
};

#endif
