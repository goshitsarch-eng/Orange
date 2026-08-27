#ifndef STRAWBERRY_APPEARANCESETTINGSPAGE_H
#define STRAWBERRY_APPEARANCESETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace AppearanceSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
