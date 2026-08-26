#ifndef STRAWBERRY_CONTEXTSETTINGSPAGE_H
#define STRAWBERRY_CONTEXTSETTINGSPAGE_H

#include <adwaita.h>

class Application;
class Settings;

namespace ContextSettingsPage {
AdwPreferencesPage *Create(Settings *settings, Application *app = nullptr);
}

#endif
