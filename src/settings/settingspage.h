#ifndef STRAWBERRY_SETTINGSPAGE_H
#define STRAWBERRY_SETTINGSPAGE_H

#include "core/settings.h"

#include <adwaita.h>

#include <functional>
#include <string>

class Application;

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon);
AdwPreferencesGroup *AddGroup(AdwPreferencesPage *page, const char *title = nullptr);

void AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback);
void AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback = "");
void AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback);
void AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked);

}  // namespace SettingsPage

#endif
