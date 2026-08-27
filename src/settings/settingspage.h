#ifndef STRAWBERRY_SETTINGSPAGE_H
#define STRAWBERRY_SETTINGSPAGE_H

#include "core/settings.h"

#include <adwaita.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

class Application;
class LoginStateWidget;

namespace SettingsPage {

AdwPreferencesPage *MakePage(const char *name, const char *icon);
AdwPreferencesGroup *AddGroup(AdwPreferencesPage *page, const char *title = nullptr);

GtkWidget *AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback,
                     const char *group_name = nullptr);
GtkWidget *AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback = "");
GtkWidget *AddPasswordEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback = "");
GtkWidget *AddIntEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, int fallback);
void AddDescription(AdwPreferencesGroup *group, const char *text, bool markup = false);
GtkWidget *AddCombo(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
                    const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
                    const std::function<void(const std::string &)> &changed = {}, const char *group_name = nullptr);
GtkWidget *AddIntCombo(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                       const std::vector<std::pair<std::string, std::string>> &choices, int fallback);
GtkWidget *AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label, const std::function<void()> &clicked,
                        const char *tooltip = nullptr);
GtkWidget *AddButtonRow(AdwPreferencesGroup *group, const char *title, const char *button_label,
                        const std::function<void(GtkWidget *button)> &clicked, const char *tooltip = nullptr);
GtkWidget *AddColorButton(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                          const char *fallback, const char *tooltip = nullptr);
void AddFontButton(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                   const char *fallback);
void AddOpacityScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                     double fallback);
GtkWidget *AddDoubleScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                          double fallback, double min, double max, double step);
GtkWidget *AddIntScale(AdwPreferencesGroup *group, Settings *settings, const char *group_name, const char *key, const char *title,
                       int fallback, int min, int max, int step);
void AddBoolRadios(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *false_title, const char *true_title,
                   bool fallback, const std::function<void(bool)> &changed = {});
void AddChoiceRadios(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title,
                     const std::vector<std::pair<std::string, std::string>> &choices, const std::string &fallback,
                     const std::function<void(const std::string &)> &changed = {});
LoginStateWidget *AddLoginState(AdwPreferencesGroup *group, Application *app, const char *service_name);

}  // namespace SettingsPage

#endif
