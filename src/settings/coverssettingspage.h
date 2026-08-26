#ifndef STRAWBERRY_COVERSSETTINGSPAGE_H
#define STRAWBERRY_COVERSSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace CoversSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
