#ifndef STRAWBERRY_GLOBALSHORTCUT_H
#define STRAWBERRY_GLOBALSHORTCUT_H

#include "core/signal.h"

#include <string>

class GlobalShortcut {
 public:
  GlobalShortcut(std::string id, std::string description, std::string default_key);

  const std::string &id() const { return id_; }
  const std::string &description() const { return description_; }
  const std::string &default_key() const { return default_key_; }
  const std::string &key() const { return key_; }
  void set_key(const std::string &key) { key_ = key; }

  Signal<> Activated;

 private:
  std::string id_;
  std::string description_;
  std::string default_key_;
  std::string key_;
};

#endif
