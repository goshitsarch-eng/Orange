#ifndef STRAWBERRY_RADIOSETTINGSPAGE_H
#define STRAWBERRY_RADIOSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace RadioSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
