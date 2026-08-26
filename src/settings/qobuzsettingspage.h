#ifndef STRAWBERRY_QOBUZSETTINGSPAGE_H
#define STRAWBERRY_QOBUZSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace QobuzSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
