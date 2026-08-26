#ifndef STRAWBERRY_GLOBALSHORTCUTSSETTINGSPAGE_H
#define STRAWBERRY_GLOBALSHORTCUTSSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace GlobalShortcutsSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
