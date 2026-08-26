#ifndef STRAWBERRY_SUBSONICSETTINGSPAGE_H
#define STRAWBERRY_SUBSONICSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace SubsonicSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
