#ifndef STRAWBERRY_TIDALSETTINGSPAGE_H
#define STRAWBERRY_TIDALSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace TidalSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
