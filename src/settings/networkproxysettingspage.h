#ifndef STRAWBERRY_NETWORKPROXYSETTINGSPAGE_H
#define STRAWBERRY_NETWORKPROXYSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace NetworkProxySettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
