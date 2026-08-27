#ifndef STRAWBERRY_SETTINGSPROVIDER_H
#define STRAWBERRY_SETTINGSPROVIDER_H

#include "core/settings.h"

class SettingsProvider {
 public:
  explicit SettingsProvider(Settings *settings) : settings_(settings) {}
  Settings *settings() const { return settings_; }

 private:
  Settings *settings_ = nullptr;
};

#endif
