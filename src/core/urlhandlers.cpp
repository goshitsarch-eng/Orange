#include "core/urlhandlers.h"

void UrlHandlers::AddHandler(UrlHandler *handler) {
  if (!handler) {
    return;
  }
  handlers_[handler->scheme()] = handler;
}

UrlHandler *UrlHandlers::HandlerForUrl(const std::string &url) const {
  const auto pos = url.find("://");
  if (pos == std::string::npos) {
    return nullptr;
  }
  const std::string scheme = url.substr(0, pos);
  const auto it = handlers_.find(scheme);
  return it == handlers_.end() ? nullptr : it->second;
}

std::vector<std::string> UrlHandlers::Schemes() const {
  std::vector<std::string> schemes;
  schemes.reserve(handlers_.size());
  for (const auto &entry : handlers_) {
    schemes.push_back(entry.first);
  }
  return schemes;
}
