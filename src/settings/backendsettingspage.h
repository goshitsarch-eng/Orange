#ifndef STRAWBERRY_BACKENDSETTINGSPAGE_H
#define STRAWBERRY_BACKENDSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace BackendSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
