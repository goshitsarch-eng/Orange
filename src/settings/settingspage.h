#ifndef STRAWBERRY_SETTINGSPAGE_H
#define STRAWBERRY_SETTINGSPAGE_H

#include "core/settings.h"

#include <adwaita.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

class Application;

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon);
AdwPreferencesGroup *AddGroup(AdwPreferencesPage *page, const char *title = nullptr);

void AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback);
void AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback = "");
void AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback);
void AddCombo(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
              const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
              const std::function<void(const std::string &)> &changed = {});
void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked);
void AddLoginState(AdwPreferencesGroup *group, Application *app, const char *service_name);

}  // namespace SettingsPage

#endif
